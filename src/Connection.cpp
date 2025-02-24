#include "webserv.hpp"

Connection::Connection(Server* server) : server(server)
{
	std::cout  << "<- accept connection ->" << std::endl;
	socklen_t addrlen = sizeof(server->sockaddr);
	fd = accept(server->socket, (struct sockaddr*)& server->sockaddr, &addrlen);
	if (fd < 0)
		throw std::runtime_error("Failed to grab connection");
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("fcntl failed");
	timeout = std::chrono::steady_clock::now() + server->request_timeout;
}


Connection::~Connection(void)
{
	std::cout << "X  close connection" << " with fd " << fd << std::endl;
	::close(fd);
}

void	Connection::receive(void)
{
	std::cout  << "<- receiving request" << std::endl;

	if (buffer.size() < BUFFER_SIZE)															// only receive new bytes if there is space in the buffer
	{
		size_t capacity = BUFFER_SIZE - buffer.size();
		if (request.header_received)
			capacity = std::min(capacity, request.remaining_bytes);								// limit capacity by content length specified in header
		std::array<char, BUFFER_SIZE> tmp;
		ssize_t	bytes_received = recv(fd, tmp.data(), BUFFER_SIZE - buffer.size(), 0);			// try to receive as many bytes as fit in the buffer
		request.remaining_bytes -= bytes_received;
		// error check
		buffer.insert(buffer.end(), tmp.begin(), tmp.begin() + bytes_received);					// append received bytes to buffer
	}

	if (!request.header_received)
	{
		request.header.append(buffer.begin(), buffer.end());
		if (request.header == "\r\n")															// ignore empty lines when expecting header firstline
			request.header.clear();
		size_t found = request.header.find("\r\n\r\n");
		if (found != std::string::npos)
		{
			request.header_received = true;

			size_t body_bytes = request.header.size() - (found + 4);							// how many bytes at the back of buffer belong to the body?
			request.header.resize(found + 2);													// resize the header to end with \r\n
			buffer.erase(buffer.begin(), buffer.begin() + (buffer.size() - body_bytes));		// remove the header bytes from the buffer so only body bytes remain
			// <<< parse request header here >>>
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

	// temporary for testing
	if (request.header.find("GET /favicon.ico HTTP") != std::string::npos)
	{
		// std::cout << "-------------------------------------------------asdf----------------------------------------------" << std::endl;
		close = true;
		return ;
	}


	if (!response.ifs_body)
	{
		// std::string filename = request.header;
		// filename.erase(filename.begin(), filename.begin() + 5);
		// filename.resize(9);
		std::string filename = "image.png";
		// std::cout << filename << std::endl;
		// std::ifstream image("index.html", std::ios::binary);
		// std::cout << "   ifstream ->" << std::endl;
		response.ifs_body = new std::ifstream(filename, std::ios::binary);
		if (!response.ifs_body || !*response.ifs_body) {
			std::cerr << "Failed to open test.png" << std::endl;
			return ;
		}
		uintmax_t size = std::filesystem::file_size(filename);

		std::ostringstream header;
		header << "HTTP/1.1 200 OK\r\n"
			<< "Content-Type: image/png\r\n"
			// << "Content-Type: text/html\r\n"
			<< "Content-Length: " << size << "\r\n"
			<< "Connection: keep-alive\r\n\r\n";
		response.header = header.str();
		buffer.insert(buffer.end(), response.header.begin(), response.header.end());
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
		//if connection not keep-alive but close, only set close=true here

		events = POLLIN;
		request = Request();
		response = Response();
		buffer.clear();
		timeout = std::chrono::steady_clock::now() + server->request_timeout;
	}
}
