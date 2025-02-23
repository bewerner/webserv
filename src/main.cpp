#include "webserv.hpp"

void	receive_request_header(Connection& connection)
{
	char buffer[BUFFER_SIZE];
	ssize_t	received = recv(connection.fd, buffer, BUFFER_SIZE, 0);
	// std::cout << "received: " << received << std::endl;
	// if (request.empty() && received < 0) {
	// 	std::cerr << "recv failed: " << strerror(errno) << std::endl;
	// }
	connection.request_header.append(buffer, received);

	// check if header is complete yet. if yes, parse it.
	size_t found = connection.request_header.find("\r\n\r\n");
	if (found != std::string::npos)
	{
		connection.request_body = connection.request_header;
		connection.request_body.erase(0, found + 4);
		connection.request_header.resize(found + 2);
		connection.request_header_received = true;

		// <<< parse request header here >>>

		if (connection.request_body.size() == connection.request_body_content_length)
			connection.request_received = true;
	}
}

void	receive_request_body(Connection& connection)
{
	(void)connection;
}

void	receive_request(Connection& connection)
{
	// std::cout  << "<- receiving request" << std::endl;
	
	if (!connection.request_header_received)
		receive_request_header(connection);
	else if (!connection.request_received)
		receive_request_body(connection);

	if (connection.request_received)
	{
		connection.timeout = std::chrono::steady_clock::now() + connection.server->request_timeout;
		connection.events = POLLOUT;
	}
	// if (connection.request_received)
	// {
	// 	static int i;
	// 	i++;
	// 	std::cout << i << std::endl;
	// }

	//debug
	if (connection.request_received)
		std::cout	 << "--------------------REQUEST-HEADER--------------------\n"
					 << connection.request_header
					 << "------------------------------------------------------\n\n\n" << std::endl;
}

void send_response(Connection& connection)
{
	// std::cout << "   Sending response ->" << std::endl;
	// temporary for testing
	if (connection.request_header.find("GET /favicon.ico HTTP") != std::string::npos)
	{
		// std::cout << "-------------------------------------------------asdf----------------------------------------------" << std::endl;
		connection.close = true;
		return ;
	}


	//buffer the file read -> define buffer size
	if (!connection.ifs_body)
	{
		// std::string filename = connection.request_header;
		// filename.erase(filename.begin(), filename.begin() + 5);
		// filename.resize(9);
		std::string filename = "image.png";
		// std::cout << filename << std::endl;
		// std::ifstream image("index.html", std::ios::binary);
		connection.buffer.clear();
		// std::cout << "   ifstream ->" << std::endl;
		connection.ifs_body = new std::ifstream(filename, std::ios::binary);
		if (!connection.ifs_body || !*connection.ifs_body) {
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
		connection.response_header = header.str();
		connection.buffer.insert(connection.buffer.end(), connection.response_header.begin(), connection.response_header.end());
	}

	if (!connection.ifs_body->eof() && connection.buffer.size() < BUFFER_SIZE)
	{
		const size_t capacity = BUFFER_SIZE - connection.buffer.size();
		std::array<char, BUFFER_SIZE> buffer;
		connection.ifs_body->read(buffer.data(), capacity);
		connection.buffer.insert(connection.buffer.end(), buffer.begin(), buffer.begin() + connection.ifs_body->gcount());
	}

	ssize_t sent = send(connection.fd, connection.buffer.data(), connection.buffer.size(), 0);
	// std::cout << "size: " << connection.buffer.size() << " sent: " << sent << " on fd " << connection.fd << " -> " << std::string(strerror(errno)) << std::endl;
	if (sent > 0)
		connection.buffer.erase(connection.buffer.begin(), connection.buffer.begin() + sent);
	// if (connection.ifs_body->eof() && connection.buffer.empty())
	// 	connection.close = true;
	if (connection.ifs_body->eof() && connection.buffer.empty())
	{
		connection.events = POLLIN;
		connection.ifs_body->close();
		delete connection.ifs_body;
		connection.buffer.clear();
		connection.ifs_body = nullptr;

		connection.request_header.clear();
		connection.request_header_received = false;
		connection.request_body_content_length = 0;
		connection.request_body.clear();
		connection.request_received = false;
		connection.timeout = std::chrono::steady_clock::now() + connection.server->request_timeout;

		// connection.close = true;
	}
}

void	init_sockets(std::vector<Server>& servers)
{
	for (Server& server : servers)
	{
		//server.init_socket() // maybe put this in Server constructor?
		server.socket = socket(PF_INET, SOCK_STREAM, 0);
		if (server.socket < 0)
			throw std::runtime_error("Failed to open socket");
		fcntl(server.socket, F_SETFL, O_NONBLOCK);
		server.sockaddr.sin_family = AF_INET;
		server.sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		server.sockaddr.sin_port = htons(server.port);

		if (bind(server.socket, (struct sockaddr*)& server.sockaddr, sizeof(sockaddr)) < 0)
			throw std::runtime_error("Failed to bind to port " + std::to_string(server.port));
		if (listen(server.socket, 1024) < 0)
			throw std::runtime_error("Failed to listen on socket");
		std::cout << "init port " << server.port << std::endl;
	}
}

void	accept_connection(std::vector<Connection>& connections, const Server& server, std::vector<pollfd>& fds)
{
	// std::cout  << "<- accept connection ->" << std::endl;
	Connection connection{};
	connection.events = POLLIN;
	socklen_t addrlen = sizeof(server.sockaddr);
	connection.fd = accept(server.socket, (struct sockaddr*)& server.sockaddr, &addrlen);
	if (connection.fd < 0)
		throw std::runtime_error("Failed to grab connection");
	fcntl(connection.fd, F_SETFL, O_NONBLOCK);
	connection.server = &server;
	connection.timeout = std::chrono::steady_clock::now() + connection.server->request_timeout;

	connections.push_back(connection);

	pollfd pollfd = { .fd = connection.fd, .events = POLLIN, .revents = 0};
	fds.push_back(pollfd);
}

void	init_fds(std::vector<pollfd>& fds, std::vector<Server>& servers)
{
	for (Server& server : servers)
	{
		pollfd pollfd = { .fd = server.socket, .events = POLLIN, .revents = 0};
		fds.push_back(pollfd);
	}
}

void sigint_handler(int signal)
{
	(void)signal;
	std::cout << "exit" << std::endl;
	exit(0);
}

int	main(int argc, char** argv)
{
	// std::atexit(test);
	// std::at_quick_exit(test);
	std::vector<Server> servers;
	std::vector<Connection> connections;
	std::vector<pollfd> fds;
	std::string path(argc > 1 ? argv[1] : "webserver.conf");
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, sigint_handler);

	parser(servers, path);

	//--------------temporary for testing--------------------------
	servers[0].port = 8080;
	servers[1].port = 8081;
	servers[2].port = 8082;
	servers[0].request_timeout = std::chrono::seconds(10);
	servers[1].request_timeout = std::chrono::seconds(10);
	servers[2].request_timeout = std::chrono::seconds(10);
	Connection padding{};
	connections.push_back(padding);
	connections.push_back(padding);
	connections.push_back(padding);
	//-------------------------------------------------------------

	init_sockets(servers);
	init_fds(fds, servers);

	while (true)
	{
		// std::cout << "servers:     " << servers.size() << std::endl;
		// std::cout << "connections: " << connections.size() - servers.size() << std::endl;

		poll(fds.data(), fds.size(), 1000);
		for (size_t i = 0; i < fds.size(); i++)
		{
			auto now = std::chrono::steady_clock::now();
			if (fds[i].revents)
			{
				if (i < servers.size())
				{

					accept_connection(connections, servers[i], fds);
					std::cout << "connections: " << connections.size() - servers.size() << std::endl;
				}
				else if (fds[i].revents & (POLLHUP | POLLERR))
					connections[i].close = true;
				else if (!connections[i].request_received)
					receive_request(connections[i]);
				// else if (!connections[i].response_sent)
				else if (fds[i].revents & POLLOUT)
					send_response(connections[i]);
			}
			else if (i >= servers.size() && connections[i].timeout <= now)
				connections[i].close = true;
			if (connections[i].close)
			{
				std::cout << "X  close connection " << errno << std::endl;
				close(connections[i].fd);
				delete connections[i].ifs_body;
				connections.erase(connections.begin() + i);
				fds.erase(fds.begin() + i);
				i--;
			}
			if (i >= servers.size())
				fds[i].events = connections[i].events;
			// for (Connection& c : connections)
			// 	std::cout << ' ' << c.fd;
			// std::cout << " ------ -1" << std::endl;
			// for (pollfd& p : fds)
			// 	std::cout << ' ' << p.fd;
			// std::cout << " ------ -1" << std::endl;
		}
	}
	// catch (const std::exception& e)
	// {
	// 	std::cerr << e.what() << (errno ? ": " + std::string(strerror(errno)) : "") << std::endl;
	// 	exit(1);
	// }
}
