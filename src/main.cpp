#include "webserv.hpp"

void	receive_request_header(Connection& connection)
{
	char buffer[BUFFER_SIZE];
	ssize_t	received = recv(connection.fd, buffer, BUFFER_SIZE, 0);
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
		connection.request_header_complete = true;

		// <<< parse request header here >>>

		if (connection.request_body.size() == connection.request_body_content_length)
			connection.request_complete = true;
	}
}

void	receive_request_body(Connection& connection)
{
	(void)connection;
}

void	receive_request(Connection& connection)
{
	std::cout  << "<- receiving request" << std::endl;
	
	if (!connection.request_header_complete)
		receive_request_header(connection);
	else if (!connection.request_complete)
		receive_request_body(connection);
	
	////debug
	if (connection.request_complete)
		std::cout	 << "--------------------REQUEST-HEADER--------------------\n"
					 << connection.request_header
					 << "------------------------------------------------------\n\n\n" << std::endl;
}

void	send_response(Connection& connection)
{
	std::cout  << "   sending response ->" << std::endl;

	std::ifstream index("index.html");
	if (!index.is_open())
		std::cerr << "failed to open" << std::endl;
	std::string body;
	std::getline(index, body, '\0');
	if (index.fail())
		std::cerr << "failed to getline" << std::endl;
	std::string content_length = std::to_string(body.size()).c_str();
	(void)content_length;
	std::ostringstream header;
	header	<<	"HTTP/1.1 200 OK\n"
			<<	"Content-Type: text/html\n"
			<<	"Content-Length: " << body.size() << '\n'
			<<	"Connection: keep-alive\n"
			<<	"\n";
	std::string response = header.str() + body;
	send(connection.fd, response.c_str(), response.size(), 0);
	connection.close = true;
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
		if (listen(server.socket, 10) < 0)
			throw std::runtime_error("Failed to listen on socket");
		std::cout << "init port " << server.port << std::endl;
	}
}

void	accept_connection(std::vector<Connection>& connections, const Server& server, std::vector<pollfd>& fds)
{
	Connection connection{};
	socklen_t addrlen = sizeof(server.sockaddr);
	connection.fd = accept(server.socket, (struct sockaddr*)& server.sockaddr, &addrlen);
	if (connection.fd < 0)
		throw std::runtime_error("Failed to grab connection");
	connection.server = &server;
	connection.last_change = std::chrono::steady_clock::now();

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
	for (size_t i = 0; i < servers.size(); i++)
		servers[i].poll_fd = fds.data() + i;
}

int	main(int argc, char** argv)
{
	std::vector<Server> servers;
	std::vector<Connection> connections;
	std::vector<pollfd> fds;
	std::string path(argc > 1 ? argv[1] : "webserver.conf");

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
		std::cout << "servers:     " << servers.size() << std::endl;
		std::cout << "connections: " << connections.size() - servers.size() << std::endl;

		poll(fds.data(), fds.size(), 1000);
		for (size_t i = 0; i < fds.size(); i++)
		{
			auto now = std::chrono::steady_clock::now();
			if (fds[i].revents)
			{
				if (i < servers.size())
					accept_connection(connections, servers[i], fds);
				else if (!connections[i].request_complete)
					receive_request(connections[i]);
				else if (!connections[i].response_complete)
					send_response(connections[i]);
				connections[i].last_change = now;
				if (connections[i].request_complete)
					fds[i].events = POLLOUT;
			}
			else if (i >= servers.size() && connections[i].last_change + connections[i].server->request_timeout <= now)
				connections[i].close = true;
			if (connections[i].close)
			{
				close(connections[i].fd);
				connections.erase(connections.begin() + i);
				fds.erase(fds.begin() + i);
				i--;
			}
		}
	}
	// catch (const std::exception& e)
	// {
	// 	std::cerr << e.what() << (errno ? ": " + std::string(strerror(errno)) : "") << std::endl;
	// 	exit(1);
	// }
}
