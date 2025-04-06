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
	for (size_t i = 0; i + 1 < buffer.size(); i++)
	{
		if (buffer[i] == '\r' && buffer[i + 1] == '\n')
			buffer.erase(buffer.begin() + i);
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
	std::cout  << "<- receiving request header" << std::endl;

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

void	Connection::check_dav_methods(void)
{
	if (response.location_config->dav_methods.find("DELETE") == response.location_config->dav_methods.end())
	{
		status_code = 405;
	}
}

void	Connection::delete_response_target(void)
{
	std::filesystem::path path(response.response_target);
	std::error_code ec;
	bool is_dir = std::filesystem::is_directory(path, ec);
	bool directory_path = response.response_target.back() == '/';

	if (ec == std::errc::permission_denied)
		status_code = 403;
	else if (ec == std::errc::no_such_file_or_directory)
		status_code = 404;
	else if (directory_path != is_dir)
		status_code = 409;

	if (status_code > 400)
		return ;

	std::filesystem::remove_all(path, ec);
	if (ec == std::errc::permission_denied)
		status_code = 500;
	else
		status_code = 204;
}

void	Connection::init_response(void)
{
	// std::cout << "   header fully received" << std::endl;
	std::cout << "   init response" << std::endl;

	response.connection = request.connection;
	if (status_code >= 300)
		response.connection = "close";

	timeout = std::chrono::steady_clock::now() + server->response_timeout;

	set_server_config();
	response.server_config = server_config;

	if (status_code < 300)
		response.set_location_config(request.request_target);

	check_dav_methods();
	if (status_code < 300)
		response.set_response_target(request.request_target, status_code, request.method);
	if (status_code < 300 && request.method == "DELETE")
		delete_response_target();
	else if (status_code < 300)
		response.init_body(status_code, request, response, *server, server->envp);
	if (status_code >= 300)
		response.init_error_body(status_code, request, *server, server->envp);
	if (!status_code)
		status_code = 200;

	response.set_status_text(status_code);
	if (status_code == 302)
		response.status_text = "Moved Temporarily";
	response.set_content_type();

	std::cout << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxXxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" << std::endl;
	std::cout << "response.cgi.pid: >" << response.cgi.pid << "<     response.cgi.fail: >" << response.cgi.fail << "<" << std::endl;
	std::cout << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxXxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" << std::endl;

	if (response.cgi.pid >= 0 && !response.cgi.fail)
		response.transfer_encoding = "chunked";
	else if (response.ifs_body)
		response.content_length = std::to_string(std::filesystem::file_size(response.response_target));
	else
	{
		if (response.str_body.empty())
			response.generate_error_page(status_code);
		response.content_length = std::to_string(response.str_body.length());
	}

	response.create_header(status_code);
}

void	Connection::receive_body(void)
{
	std::cout  << "   forwarding body to cgi -->" << std::endl;
	std::cout  << "   cgi listening?: " << response.cgi.pollout() << std::endl;


	//UNCHUNK CHUNKED BODY HERE

	// std::cout << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxRECEIVExBODYxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" << std::endl;
	size_t size = std::min(buffer.size(), request.remaining_bytes);
	ssize_t sent = write(response.cgi.pipe_into_cgi[1], buffer.data(), size);
	if (sent == -1)
	{
		std::cout << "write to cgi FAIL" << std::endl;
	}
	if (sent == -1 && !response.cgi.is_running())
	{
		std::cout << "write to cgi FAIL" << std::endl;
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
	std::cout  << "   --> sent " << sent << " bytes to cgi: " << strerror(errno) << std::endl;

	std::cout  << "remaining_bytes " << request.remaining_bytes << "     buffer size: " << buffer.size() << std::endl;

	if (request.remaining_bytes == 0 && buffer.empty())
	{
		response.cgi.done_writing_into_cgi();
		request.received = true;
		std::cout << "done writing into cgi" << std::endl;
		// exit (0);
	}
}

void	Connection::receive(void)
{
	std::cout << "<- connection receive" << std::endl;
	std::cout << "   content length: " << request.content_length << std::endl;
	std::cout << "   remaining bytes: " << request.remaining_bytes << std::endl;
	std::cout << "   buffer size: " << buffer.size() << std::endl;
	std::cout << "   listening to client?: ";
	if (events == POLLIN)
		std::cout << "yes" << std::endl;
	else
		std::cout << "no" << std::endl;
	std::cout << "   client speaking?: " << (*revents & POLLIN) << std::endl;
	// sleep(1);

	if (buffer.size() < BUFFER_SIZE && *revents & POLLIN)																// only receive new bytes if there is space in the buffer
	{
		size_t capacity = BUFFER_SIZE - buffer.size();
		// if (request.header_received && request.content_length_specified)
		// 	capacity = std::min(capacity, request.remaining_bytes);									// limit capacity by content length specified in header
		if (capacity)
		{
			std::array<char, BUFFER_SIZE> tmp;
			ssize_t	bytes_received = recv(fd, tmp.data(), capacity, 0);				// try to receive as many bytes as fit in the buffer
			std::cout << "   received from client: " << bytes_received << std::endl;
			std::cout << "   received from client: " << strerror(errno) << std::endl;
			if (bytes_received > 0)
			{
				// if (request.header_received && request.content_length_specified)
				// 	request.remaining_bytes -= bytes_received;
				buffer.insert(buffer.end(), tmp.begin(), tmp.begin() + bytes_received);						// append received bytes to buffer
			}
		}
	}

	if (!request.header_received)
	{
		receive_header();
		if (request.header_received)
			init_response();
	}

	// std::cout << "\n\n\nheader received: " << request.header_received << "    method: " << request.method << "     pollout: " << response.cgi.pollout() << std::endl << std::endl << std::endl;

	if (!buffer.empty() && request.header_received && request.method == "POST" && response.cgi.pollout())
		receive_body();
	if (!request.received && request.header_received && request.method == "POST" && !response.cgi.is_running())
	{
		response.cgi.fail = true;
		status_code = 500;
		init_response();
		request.received = true;
	}

	if (request.header_received && !request.received && request.method != "POST")
	{
		std::cout << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXsettingtrue";
		request.received = true;
	}


	if (request.received)
	{
		response.cgi.done_writing_into_cgi();
		std::cout << "   request fully received" << std::endl;
		timeout = std::chrono::steady_clock::now() + server->response_timeout;
		events = POLLOUT;
		// exit(0);
	}
}

void	Connection::respond(void)
{
	// if (!response.cgi.header_extracted && !response.cgi.pollin())
	// 	return ;
	// if (!response.cgi.pollin() && buffer.empty())
	// 	return ;
	// sleep(1);
	std::cout << "   Sending response ->" << std::endl;
	// if (response.cgi.pipe_from_cgi[0] >= 0 && response.cgi.revents_read_from_cgi)
	// 	std::cout << "   revents_read_from_cgi POLLIN:  " << (*response.cgi.revents_read_from_cgi & POLLIN) << std::endl;
	// else
	// 	std::cout << "   read from cgi is closed  " << std::endl;
	// std::cout << "   revents connection    POLLOUT: " << (*revents & POLLOUT) << std::endl;
	// std::cout << "   response header sent: " << response.header_sent << std::endl;

	// if (response.header.empty())
	if (!response.header_sent)
	{
		buffer.clear();
		buffer.insert(buffer.begin(), response.header.begin(), response.header.end());
		response.header_sent = true;
		if (!response.str_body.empty())
			buffer.insert(buffer.end(), response.str_body.begin(), response.str_body.end());

		// debug
		std::cout	<< "--------------------RESPONSE-HEADER--------------------\n"
					<< response.header
					<< "------------------------------------------------------\n\n\n" << std::endl;
		// if (!response.str_body.empty())
		// 	exit(0);
	}

	bool using_cgi = (response.cgi.pid >= 0 && !response.cgi.fail);
	if (using_cgi && response.cgi.pollin() && !response.cgi.eof)
	{
		std::cout << "<- reading from cgi" << std::endl;
		const size_t capacity = BUFFER_SIZE - buffer.size();
		std::array<char, BUFFER_SIZE> buf;
		ssize_t received = read(response.cgi.pipe_from_cgi[0], &buf, capacity);
		std::cout << "   received from cgi: " << received << "   capacity: " << capacity << std::endl;
		// if (!received)
		// {
		// 	close = true;
		// 	return ;
		// }
		if (!received && !response.cgi.header_extracted)
		{
			std::cout << "   cgi reached EOF before receiving the cgi_header -> 500" << std::endl;
			// response.cgi.eof = true;
			response.cgi.fail = true;
			status_code = 500;
			response.status_text = "Internal Server Error";
			response.generate_error_page(status_code);
			response.connection = "close";
			response.create_header(status_code);
			return ;
		}
		if (!received && response.cgi.header_extracted)
		{
			std::cout << "   cgi reached EOF" << std::endl;
			response.cgi.done_reading_from_cgi();
		}
		if (received > 0 && !response.cgi.header_extracted)
		{
			std::cout << "   extracting cgi header" << std::endl;
			response.extract_cgi_header(buf, received, status_code);
			if (!response.cgi.fail && response.cgi.header_extracted)
			{
				std::cout << "   cgi header extracted" << std::endl;
				buffer.clear();
				buffer.insert(buffer.begin(), response.header.begin(), response.header.end());
				response.header_sent = true;
			}
			if (!received || response.cgi.fail)
				return ;
		}

		// for (char c : buf)
		// std::cout << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxXxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" << std::endl;
		//CREATE CHUNK HEAD
		if (response.cgi.header_extracted)
		{
			std::cout << "   writing a chunk with " << received << " bytes into cgi body buffer" << std::endl;

			std::string chunk_size = (std::ostringstream{} << std::hex << received << "\r\n").str();
			buffer.insert(buffer.end(), chunk_size.begin(), chunk_size.end());
			buffer.insert(buffer.end(), buf.begin(), buf.begin() + received);
			static const std::string chunk_end = "\r\n\r\n";
			buffer.insert(buffer.end(), chunk_end.begin(), chunk_end.end());
			if (!received)
				response.cgi.done_reading_from_cgi();
		}
	}
	if (using_cgi && (!response.cgi.header_extracted || response.cgi.fail))
		return ;
	// else if (using_cgi)
	// 	return ;
	else if (response.ifs_body && !response.ifs_body->eof() && buffer.size() < BUFFER_SIZE)
	{
		const size_t capacity = BUFFER_SIZE - buffer.size();
		std::array<char, BUFFER_SIZE> buf;
		response.ifs_body->read(buf.data(), capacity);
		buffer.insert(buffer.end(), buf.begin(), buf.begin() + response.ifs_body->gcount());
	}

	if (using_cgi && !response.cgi.header_extracted)
		return ;

	if (*revents & POLLOUT)
	{
		ssize_t sent = send(fd, buffer.data(), buffer.size(), 0);
		std::cout << "   sent " << sent << " bytes to client    buffersize: " << buffer.size() << std::endl;
		if (sent > 0)
			buffer.erase(buffer.begin(), buffer.begin() + sent);
	}


	// if (using_cgi)
	// {
	// 	if (waitpid(response.cgi.pid, NULL, WNOHANG) == response.cgi.pid)
	// 		response.cgi.eof = true;
	// }
	std::cout << "using cgi:  " << using_cgi << std::endl;
	std::cout << "cgi.eof:    " << response.cgi.eof << std::endl;
	std::cout << "buffersize: " << buffer.size() << std::endl;
	std::cout << "pollout: "    << (*revents & POLLOUT) << std::endl;

	if ((!response.ifs_body || response.ifs_body->eof()) && (!using_cgi || response.cgi.eof) && buffer.empty())
	{
		std::cout << "✓ response fully sent  " << std::endl;
		std::cout << "✓ close: " << close << std::endl;
		// // debug
		// if (request.header_received || request.received)
		// {
		// 	std::cout	 << "--------------------REQUEST-HEADER--------------------\n"
		// 				<< request.header
		// 				<< "------------------------------------------------------\n\n\n" << std::endl;
		// }
		// exit(0);
		if (response.connection != "keep-alive")
			close = true;

		events = POLLIN;
		request = Request();
		response = Response();
		buffer.clear();
		timeout = std::chrono::steady_clock::now() + server->request_timeout;
		status_code = 0;
		std::cout << "✓ close: " << close << std::endl;
	}
}
