#include "webserv.hpp"

Connection::Connection(Server* server) : server(server)
{
	// std::cout  << "<- accept connection ->" << std::endl;
	socklen_t addrlen = sizeof(sockaddr);
	fd = accept(server->socket, (struct sockaddr*)&sockaddr, &addrlen);
	int flag = 1;
	if (fd < 0 || fcntl(fd, F_SETFL, O_NONBLOCK) < 0 || setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) < 0)
	{
		std::cerr << "ERROR: failed to accept connection: " << strerror(errno);
		close = true;
		return ;
	}
	timeout = std::chrono::steady_clock::now() + server->request_timeout;
}

Connection::~Connection(void)
{
	// std::cout << "X  close connection with fd " << fd << std::endl;
	// if (fd >= 0)
	// 	::close(fd);
	if (fd >= 0)
	{
		// std::cout << "depleting" << std::endl;
		// static std::array<char, BUFFER_SIZE> tmp;
		// for (size_t i = 0; i < 9999999; i++)
		// 	recv(fd, tmp.data(), BUFFER_SIZE, 0);
		// std::cout << std::endl;
		// std::cout << "depleted" << std::endl;

		// sleep(5);
		// sleep(1);
		::close(fd);
	}
}

static void	normalize_line_feed(std::vector<char>& buffer)
{
	for (size_t i = 0; i + 1 < buffer.size(); i++)
	{
		if (buffer[i] == '\r' && buffer[i + 1] == '\n')
			buffer.erase(buffer.begin() + i);
		if (i >= 1 && buffer[i] == '\n' && buffer[i - 1] == '\n')
			break ;
	}
}

void	Connection::set_server_config(void)
{
	for (const ServerConfig& config : server->conf)
	{
		if (config.server_name == request.host)
		{
			server_config = &(config);
			return ;
		}
	}
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
		request.header_received = true; // TEMP
	}
	else
		buffer.clear();

	// debug
	if (request.header_received || request.received)
	{
		std::cout	 << "--------------------REQUEST-HEADER--------------------\n"
					 << request.header
					 << "------------------------------------------------------\n\n\n" << std::endl;
	}
}

void	Connection::validate_method(void)
{
	if (request.method == "DELETE" && response.location_config->dav_methods != "DELETE")
		status_code = 405;
	else if (request.method == "POST" && response.location_config->cgi == false)
	{
		status_code = 405;
		response.connection = "close";
	}
	else if (request.method == "POST" && request.content_length > response.location_config->client_max_body_size)
	{
		status_code = 413;
		response.connection = "close";
	}
}

void	Connection::delete_response_target(void)
{
	std::filesystem::path path(response.response_target);
	std::error_code ec;
	bool is_dir = std::filesystem::is_directory(path, ec);
	bool directory_path = response.response_target.back() == '/';

	std::filesystem::path parent = path.parent_path();
	if (directory_path)
		parent = parent.parent_path();

	if (ec == std::errc::permission_denied)
		status_code = 403;
	else if (ec == std::errc::no_such_file_or_directory)
		status_code = 404;
	else if (directory_path != is_dir)
		status_code = 409;
	else if (ec.value())
		status_code = 500;
	else if (access(parent.c_str(), W_OK) != 0)
		status_code = 403;

	if (status_code >= 400)
		return ;

	std::filesystem::remove_all(path, ec);
	if (ec.value())
		status_code = 500;
	else
		status_code = 204;
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
		validate_method();
	if (status_code < 300)
		response.set_response_target(request.request_target, status_code, request.method);
	if (status_code < 300 && request.method == "DELETE")
		delete_response_target();
	else if (status_code < 300)
		response.init_body(status_code, request, response, *server, *this);
	if (status_code >= 300)
		response.init_error_body(status_code, request, *server, *this);
	if (!status_code)
		status_code = 200;

	response.set_status_text(status_code);
	if (status_code == 302)
		response.status_text = "Moved Temporarily";
	response.set_content_type();

	// if (response.cgi.pid >= 0 && !response.cgi.fail)
	// 	response.transfer_encoding = "chunked";
	// else if (response.ifs_body)
	// {
	// 	response.content_length = std::to_string(std::filesystem::file_size(response.response_target));
	// 	response.transfer_encoding.clear();
	// }
	// else if (status_code >= 300)
	// {
	// 	if (response.str_body.empty())
	// 		response.generate_error_page(status_code);
	// 	response.content_length = std::to_string(response.str_body.length());
	// 	response.transfer_encoding.clear();
	// }

	response.create_header(status_code);
}

void	Connection::receive_body(void)
{
	if (status_code >= 300)
		return ;

	size_t size = std::min(buffer.size(), request.remaining_bytes);
	ssize_t sent = write(response.cgi.pipe_into_cgi[1], buffer.data(), size);
	if (sent == -1 && !response.cgi.is_running())
	{
		std::cerr << "write to cgi FAIL" << std::endl;
		response.cgi.fail = true;
		request.received = true;
		status_code = 500;
		return ;
	}
	if (sent > 0)
	{
		request.remaining_bytes -= sent;
		buffer.erase(buffer.begin(), buffer.begin() + sent);
	}
	if (request.remaining_bytes == 0 && buffer.empty())
	{
		response.cgi.done_writing_into_cgi();
		request.received = true;
	}
}

void	Connection::receive(void)
{
	if (buffer.size() < BUFFER_SIZE && pollin())												// only receive new bytes if there is space in the buffer
	{
		size_t capacity = BUFFER_SIZE - buffer.size();
		if (request.header_received && request.content_length_specified)
			capacity = std::min(capacity, request.remaining_bytes);								// limit capacity by content length specified in header
		if (capacity)
		{
			static std::array<char, BUFFER_SIZE> tmp;
			ssize_t	bytes_received = recv(fd, tmp.data(), capacity, 0);							// try to receive as many bytes as fit in the buffer
			if (bytes_received > 0)
				buffer.insert(buffer.end(), tmp.begin(), tmp.begin() + bytes_received);			// append received bytes to buffer
		}
	}
	if (!request.header_received)
	{
		receive_header();
		if (request.header_received || request.received)
			init_response();
	}
	if (!buffer.empty() && request.header_received && request.method == "POST" && response.cgi.pollout())
		receive_body();
	if (!request.received && request.header_received && request.method == "POST" && !response.cgi.is_running())
		request.received = true;
	if (request.header_received && !request.received && request.method != "POST")
		request.received = true;
	if (request.received || status_code >= 300)
	{
		buffer.clear();
		response.cgi.done_writing_into_cgi();
		timeout = std::chrono::steady_clock::now() + server->response_timeout;
		events = POLLOUT;
	}
}

void	Connection::respond(void)
{
	if (!response.header_sent)
	{
		buffer.clear();
		buffer.insert(buffer.begin(), response.header.begin(), response.header.end());
		response.header_sent = true;
		if (!response.str_body.empty())
			buffer.insert(buffer.end(), response.str_body.begin(), response.str_body.end());

		// debug
		std::cout	<< "--------------------SENDING-RESPONSE-HEADER--------------------\n"
					<< response.header
					<< "---------------------------------------------------------------\n\n\n" << std::endl;
	}
	bool using_cgi = (response.cgi.pid >= 0 && !response.cgi.fail);
	if (using_cgi && response.cgi.pollin() && !response.cgi.eof)
	{
		const size_t capacity = BUFFER_SIZE - buffer.size();
		static std::array<char, BUFFER_SIZE> buf;
		ssize_t received = read(response.cgi.pipe_from_cgi[0], &buf, capacity);
		if (!received && !response.cgi.header_extracted)
			throw std::runtime_error("   cgi sent EOF before sending the cgi_header -> 500");
		if (!received && response.cgi.header_extracted)
			response.cgi.done_reading_from_cgi();
		if (received > 0 && !response.cgi.header_extracted)
		{
			response.extract_cgi_header(buf, received, status_code);
			if (!response.cgi.fail && response.cgi.header_extracted)
			{
				buffer.clear();
				buffer.insert(buffer.begin(), response.header.begin(), response.header.end());
				response.header_sent = true;
			}
			if (!received || response.cgi.fail)
				return ;
		}
		if (response.cgi.header_extracted)
		{
			std::string chunk_size = (std::ostringstream{} << std::hex << received << "\r\n").str();
			buffer.insert(buffer.end(), chunk_size.begin(), chunk_size.end());
			buffer.insert(buffer.end(), buf.begin(), buf.begin() + received);
			static const std::string chunk_end = "\r\n";
			buffer.insert(buffer.end(), chunk_end.begin(), chunk_end.end());
			if (!received)
				response.cgi.done_reading_from_cgi();
		}
	}
	if (using_cgi && (!response.cgi.header_extracted || response.cgi.fail))
		return ;
	else if (response.ifs_body && !response.ifs_body->eof() && buffer.size() < BUFFER_SIZE)
	{
		const size_t capacity = BUFFER_SIZE - buffer.size();
		static std::array<char, BUFFER_SIZE> buf;
		response.ifs_body->read(buf.data(), capacity);
		buffer.insert(buffer.end(), buf.begin(), buf.begin() + response.ifs_body->gcount());
	}
	if (using_cgi && !response.cgi.header_extracted)
		return ;

	if (pollout())
	{
		ssize_t sent = send(fd, buffer.data(), buffer.size(), 0);
		if (sent > 0)
			buffer.erase(buffer.begin(), buffer.begin() + sent);
	}
	if ((!response.ifs_body || response.ifs_body->eof()) && (!using_cgi || response.cgi.eof) && buffer.empty())
	{
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

void	Connection::handle_exception(const std::exception& e)
{
	try
	{
		std::cerr << "exception: " << e.what() << std::endl;
		if (exception)
		{
			close = true;
			return ;
		}

		exception = true;
		response.cgi.fail = true;
		events = POLLOUT;
		if (auto* fs_error = dynamic_cast<const std::filesystem::filesystem_error*>(&e))
			status_code = 403;
		else
			status_code = 500;
		response.set_status_text(status_code);
		response.init_error_body(status_code, request, *server, *this);
		// response.generate_error_page(status_code);
		response.create_header(status_code);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		close = true;
	}
}

void	Connection::handle_timeout(void)
{
	try
	{
		std::cerr << "timeout" << std::endl;
		shutdown(fd, SHUT_RD);
		if (status_code == 408 || status_code == 504)// || response.header_sent)
		{
			close = true;
			return ;
		}
		response.cgi.fail = true;

		// std::cout << "depleting" << std::endl;
		// static std::array<char, BUFFER_SIZE> tmp;
		// for (size_t i = 0; i < 9999999; i++)
		// 	recv(fd, tmp.data(), BUFFER_SIZE, 0);
		// std::cout << std::endl;
		// std::cout << "depleted" << std::endl;

		if (events == POLLOUT)
			status_code = 504; // Gateway Timeout
		else if (!request.header_received)
		{
			close = true;
			return ;
		}
		else
		{
			status_code = 408; // Request Timeout
			response.connection = "close";
		}
		events = POLLOUT;
		response.set_status_text(status_code);
		response.init_error_body(status_code, request, *server, *this);
		// response.generate_error_page(status_code);
		response.create_header(status_code);
		response.connection = "close";
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		close = true;
	}
}

bool	Connection::pollin(void)
{
	return (revents && *revents & POLLIN);
}

bool	Connection::pollout(void)
{
	return (revents && *revents & POLLOUT);
}

bool	Connection::pollhup(void)
{
	return (revents && *revents & POLLHUP);
}

bool	Connection::pollerr(void)
{
	return (revents && *revents & POLLERR);
}
