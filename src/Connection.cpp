#include "webserv.hpp"
#include <filesystem>
namespace fs = std::filesystem;

Connection::Connection(Server* server) : server(server)
{
	std::cout  << "<- accept connection ->" << std::endl;
	socklen_t addrlen = sizeof(server->sockaddr);
	fd = accept(server->socket, (struct sockaddr*)& server->sockaddr, &addrlen);
	if (fd < 0)
		throw std::runtime_error("Failed to grab connection");

	// int opt = 1;
	// setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	// struct linger linger_opt = { 0, 0 }; // Linger active, timeout 0 seconds
	// setsockopt(fd, SOL_SOCKET, SO_LINGER, &linger_opt, sizeof(linger_opt));

	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("fcntl failed");
	timeout = std::chrono::steady_clock::now() + server->request_timeout;
}

Connection::~Connection(void)
{
	std::cout << "X  close connection" << " with fd " << fd << std::endl;
	::close(fd);
}

static void	normalize_line_feed(std::vector<char>& buffer)
{
	// for (size_t i = 0; i < buffer.size(); i++)
	// {
	// 	if (buffer[i] == '\n' && (i == 0 || buffer[i - 1] != '\r'))
	// 		buffer.insert(buffer.begin() + i, '\r');
	// 	if (i >= 3 && buffer[i] == '\n' && buffer[i - 1] == '\r' && buffer[i - 2] == '\n' && buffer[i - 3] == '\r')
	// 		break ;
	// }
	for (size_t i = 1; i < buffer.size(); i++)
	{
		if (buffer[i] == '\n' && buffer[i - 1] == '\r')
			buffer.erase(buffer.begin() + i - 1);
		if (i >= 1 && buffer[i] == '\n' && buffer[i - 1] == '\n')
			break ;
	}
}

void	Connection::set_config(void)
{
	for (const auto& server_config : server->conf)
	{
		if (server_config.server_name == request.host)
		{
			std::cout << "server config: " << server_config.server_name
				<< " | host is matching" << std::endl;
			config = &(server_config);
			return ;
		}
	}
	std::cout << "server config: " << server->conf[0].server_name
		<< " | default" << std::endl;
	config = &(server->conf[0]);
}

void	Connection::receive(void)
{
	std::cout  << "<- receiving request" << std::endl;

	if (buffer.size() < BUFFER_SIZE)																// only receive new bytes if there is space in the buffer
	{
		size_t capacity = BUFFER_SIZE - buffer.size();
		if (request.header_received)
			capacity = std::min(capacity, request.remaining_bytes);									// limit capacity by content length specified in header
		std::array<char, BUFFER_SIZE> tmp;
		ssize_t	bytes_received = recv(fd, tmp.data(), BUFFER_SIZE - buffer.size(), 0);				// try to receive as many bytes as fit in the buffer
		request.remaining_bytes -= bytes_received;
		// error check
		buffer.insert(buffer.end(), tmp.begin(), tmp.begin() + bytes_received);						// append received bytes to buffer
	}

	if (!request.header_received)
	{
		if (buffer.empty() || buffer.front() == '\xFF' || buffer.front() == '\x04')					// close without response (for example when pressing ctrl+c in telnet)
		{
			close = true;
			return ;
		}

		normalize_line_feed(buffer);																// replaces "\r\n" with "\n"
		request.header.append(buffer.begin(), buffer.end());
		while (request.header.find('\n') == 0)														// ignore empty lines when expecting header firstline
		{
			request.header.erase(request.header.begin());
			buffer.erase(buffer.begin());
		}

		if (!request.startline_parsed && request.header.find('\n') != std::string::npos)			// startline received -> parse it
		{
			std::istringstream iss_header(request.header);
			parse_start_line(request, iss_header, status_code);
			request.startline_parsed = true;
			if (status_code == 400 || status_code == 505)
				request.received = true;
		}
		if (size_t found = request.header.find("\n\n"); found != std::string::npos)
		{
			request.header_received = true;

			size_t body_bytes = request.header.size() - (found + 2);								// how many bytes at the back of buffer belong to the body?
			request.header.resize(found + 1);														// resize the header to end with \n
			buffer.erase(buffer.begin(), buffer.begin() + (buffer.size() - body_bytes));			// remove the header bytes from the buffer so only body bytes remain
			parse_request(request, status_code);
			request.received = true; // TEMP
		}
		else
			buffer.clear();
	}
	// else
	// {
	// 	//write body
	// }

	if (request.received)
	{
		timeout = std::chrono::steady_clock::now() + server->request_timeout;
		events = POLLOUT;
		buffer.clear();
	}

	// debug
	if (request.received)
	{
		std::cout	 << "--------------------REQUEST-HEADER--------------------\n"
					 << request.header
					 << "------------------------------------------------------\n\n\n" << std::endl;
	}
}

void	Connection::respond(void)
{
	std::cout << "   Sending response ->" << std::endl;

	if (!response.ifs_body && response.directory_listing.empty())
	{
		timeout = std::chrono::steady_clock::now() + server->response_timeout;

		if (!status_code)
			status_code = 200;
		response.connection = request.connection;

		set_config();
		response.config = config;
		response.set_location(request.request_target);

		response.set_body_path(status_code, request.request_target);
		response.set_status_text(status_code);
		response.set_content_type();

		if (status_code >= 400)
			response.connection = "close";

		response.ifs_body = std::make_shared<std::ifstream>(response.body_path, std::ios::binary);
		if (!response.ifs_body || !response.ifs_body->is_open())
		{
			close = true;
			return ;
		}
		uintmax_t size = std::filesystem::file_size(response.body_path);

		std::ostringstream header;
		header	<< "HTTP/1.1 " << status_code << ' ' << response.status_text << "\r\n"
				<< "Content-Type: " << response.content_type << "\r\n"
				<< "Content-Length: " << size << "\r\n"
				<< "Connection: " << response.connection << "\r\n\r\n";
		response.header = header.str();
		buffer.insert(buffer.end(), response.header.begin(), response.header.end());

			// debug
			std::cout	<< "--------------------RESPONSE-HEADER--------------------\n"
						<< header.str()
						<< "------------------------------------------------------\n\n\n" << std::endl;
	}

	if (!response.ifs_body->eof() && buffer.size() < BUFFER_SIZE)
	{
		const size_t capacity = BUFFER_SIZE - buffer.size();
		std::array<char, BUFFER_SIZE> tmp;
		response.ifs_body->read(tmp.data(), capacity);
		buffer.insert(buffer.end(), tmp.begin(), tmp.begin() + response.ifs_body->gcount());
	}

	ssize_t sent = send(fd, buffer.data(), buffer.size(), 0);
	// std::cout << "size: " << buffer.size() << " sent: " << sent << " on fd " << fd << " -> " << std::string(strerror(errno)) << std::endl;
	if (sent > 0)
		buffer.erase(buffer.begin(), buffer.begin() + sent);
	// if (response.ifs_body->eof() && buffer.empty())
	// 	close = true;
	if (response.ifs_body->eof() && buffer.empty())
	{
		if (response.connection != "keep-alive")
			close = true;
		//if connection not keep-alive but close, only set close=true here

		events = POLLIN;
		request = Request();
		response = Response();
		buffer.clear();
		timeout = std::chrono::steady_clock::now() + server->request_timeout;
	}
}
