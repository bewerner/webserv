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
	for (size_t i = 1; i < buffer.size(); i++)
	{
		if (buffer[i] == '\n' && buffer[i - 1] == '\r')
			buffer.erase(buffer.begin() + i - 1);
		if (i >= 1 && buffer[i] == '\n' && buffer[i - 1] == '\n')
			break ;
	}
}

// static void	normalize_line_feed(std::string& str)
// {
// 	for (size_t i = 1; i < str.size(); i++)
// 	{
// 		if (str[i] == '\n' && str[i - 1] == '\r')
// 			str.erase(str.begin() + i - 1);
// 		if (i >= 1 && str[i] == '\n' && str[i - 1] == '\n')
// 			break ;
// 	}
// }

void	Connection::set_server_config(void)
{
	for (const ServerConfig& config : server->conf)
	{
		if (config.server_name == request.host)
		{
			std::cout << "server config: " << config.server_name
				<< " | host is matching" << std::endl;
			server_config = &(config);
			return ;
		}
	}
	std::cout << "server config: " << server->conf[0].server_name
		<< " | default" << std::endl;
	server_config = &(server->conf[0]);
}

void	Connection::receive_header(void)
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

	if (request.received)
	{
		timeout = std::chrono::steady_clock::now() + server->request_timeout;
		events = POLLOUT;
	}

	// debug
	if (request.received)
	{
		std::cout	 << "--------------------REQUEST-HEADER--------------------\n"
					 << request.header
					 << "------------------------------------------------------\n\n\n" << std::endl;
	}
}

void	Connection::init_response(void)
{
	response.connection = request.connection;
	if (status_code >= 300)
		response.connection = "close";

	timeout = std::chrono::steady_clock::now() + server->response_timeout;

	set_server_config();
	response.server_config = server_config;

	if (status_code < 300)
		response.set_location_config(request.request_target);

	if (status_code < 300)
		response.set_response_target(request.request_target, status_code);
	if (status_code < 300)
		response.init_body(status_code, request, server->port);
	if (status_code >= 300)
		response.init_error_body(status_code, request, server->port);
	if (!status_code)
		status_code = 200;

	response.set_status_text(status_code);
	if (status_code == 302)
		response.status_text = "Moved Temporarily";
	response.set_content_type();

	if (response.ifs_body)
		response.content_length = std::to_string(std::filesystem::file_size(response.response_target));
	else
	{
		if (response.str_body.empty())
			response.generate_error_page(status_code);
		// buffer.insert(buffer.begin(), response.str_body.begin(), response.str_body.end());
	}

	response.create_header(status_code);
}

void	Connection::receive(void)
{
	std::cout  << "<- receiving request header" << std::endl;

	if (buffer.size() < BUFFER_SIZE)																// only receive new bytes if there is space in the buffer
	{
		size_t capacity = BUFFER_SIZE - buffer.size();
		if (request.header_received && request.content_length)
			capacity = std::min(capacity, request.content_length);									// limit capacity by content length specified in header
		if (capacity)
		{
			std::array<char, BUFFER_SIZE> tmp;
			ssize_t	bytes_received = recv(fd, tmp.data(), BUFFER_SIZE - buffer.size(), 0);				// try to receive as many bytes as fit in the buffer
			request.remaining_bytes -= bytes_received;
			// error check
			buffer.insert(buffer.end(), tmp.begin(), tmp.begin() + bytes_received);						// append received bytes to buffer
		}
	}

	if (!request.header_received)
	{
		receive_header();
		if (request.header_received)
			init_response();
	}
	
	// if (request.header_received)
	// 	receive_body();
}

void	Connection::respond(void)
{
	std::cout << "   Sending response ->" << std::endl;

	// if (response.header.empty())
	if (!response.header_sent)
	{

		// response.connection = request.connection;
		// if (status_code >= 300)
		// 	response.connection = "close";

		// timeout = std::chrono::steady_clock::now() + server->response_timeout;

		// set_server_config();
		// response.server_config = server_config;

		// if (status_code < 300)
		// 	response.set_location_config(request.request_target);

		// if (status_code < 300)
		// 	response.set_response_target(request.request_target, status_code);
		// if (status_code < 300)
		// 	response.init_body(status_code, request, server->port);
		// if (status_code >= 300)
		// 	response.init_error_body(status_code, request, server->port);
		// if (!status_code)
		// 	status_code = 200;

		// response.set_status_text(status_code);
		// if (status_code == 302)
		// 	response.status_text = "Moved Temporarily";
		// response.set_content_type();

		// if (response.ifs_body)
		// 	response.content_length = std::to_string(std::filesystem::file_size(response.response_target));
		// else
		// {
		// 	if (response.str_body.empty())
		// 		response.generate_error_page(status_code);
		// 	buffer.insert(buffer.begin(), response.str_body.begin(), response.str_body.end());
		// }

		// response.create_header(status_code);
		buffer.clear();
		buffer.insert(buffer.begin(), response.header.begin(), response.header.end());
		response.header_sent = true;
		if (!response.str_body.empty())
			buffer.insert(buffer.end(), response.str_body.begin(), response.str_body.end());

		// debug
		std::cout	<< "--------------------RESPONSE-HEADER--------------------\n"
					<< response.header
					<< "------------------------------------------------------\n\n\n" << std::endl;
	}

	if (response.ifs_body && !response.ifs_body->eof() && buffer.size() < BUFFER_SIZE)
	{
		const size_t capacity = BUFFER_SIZE - buffer.size();
		std::array<char, BUFFER_SIZE> tmp;
		response.ifs_body->read(tmp.data(), capacity);
		buffer.insert(buffer.end(), tmp.begin(), tmp.begin() + response.ifs_body->gcount());
	}

	ssize_t sent = send(fd, buffer.data(), buffer.size(), 0);
	if (sent > 0)
		buffer.erase(buffer.begin(), buffer.begin() + sent);

	if ((!response.ifs_body || response.ifs_body->eof()) && buffer.empty())
	{
		std::cout << "✓ response fully sent  " << std::endl;
		if (response.connection != "keep-alive")
			close = true;

		events = POLLIN;
		request = Request();
		response = Response();
		buffer.clear();
		timeout = std::chrono::steady_clock::now() + server->request_timeout;
		status_code = 0;
	}
}
