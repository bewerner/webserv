#include "webserv.hpp"

std::string	receive_request(int fd)
{
	std::string request;
	char buffer[1024];
	ssize_t	received;
	ssize_t	ready;

	while ((ready = recv(fd, buffer, sizeof(buffer), MSG_PEEK)) < 0)
	{
		std::cout << "waiting" << std::endl;
		usleep(100000);
	}

	while ((received = recv(fd, buffer, sizeof(buffer), 0)) > 0)
		request.append(buffer, received);

	if (request.empty() && received < 0) {
		std::cerr << "recv failed: " << strerror(errno) << std::endl;
	}

	return (request);
}

void	handle_request(const Server& server)
{
	std::cout << "handle request on port " << server.port << std::endl;
	socklen_t addrlen = sizeof(server.sockaddr);
	int connection = accept(server.socket, (struct sockaddr*)& server.sockaddr, &addrlen);
	if (connection < 0)
		throw std::runtime_error("Failed to grab connection");

	// sleep(1);

	std::string request = receive_request(connection);
	if (request.empty())
		return ;
	std::cout	<< "\n\n\n"
				<< "--------------------REQUEST--------------------\n"
				<< request << "\n"
				<< "-----------------------------------------------\n\n\n" << std::endl;


	// char buffer[20000];
	// (void)read(connection, buffer, 20000);
	// std::cout << "The message was: " << buffer << std::endl;

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
	send(connection, response.c_str(), response.size(), 0);
	close(connection);
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
			throw std::runtime_error("Failed to bind to port 9999");
		if (listen(server.socket, 10) < 0)
			throw std::runtime_error("Failed to listen on socket");
	}
}

void	init_pollfds(std::vector<pollfd>& fds, std::vector<Server>& servers)
{
	for (Server& server : servers)
	{
		pollfd pollfd = { .fd = server.socket, .events = POLLIN, .revents = 0};
		fds.push_back(pollfd);
	}
	for (size_t i = 0; i < servers.size(); i++)
		servers[i].poll_fd = fds.data() + i;
}

int	poll_servers(std::vector<Server>& servers)
{
		static std::vector<pollfd> fds;
		if (fds.empty())
			init_pollfds(fds, servers);

		std::cout << "polling" << std::endl;
		int status = poll(fds.data(), fds.size(), 3000);
		std::cout << "done polling" << std::endl;
		if (status < 0)
			std::cerr << "poll error" << std::endl;
		if (status == 0)
			std::cout << "0" << std::endl;

		return (status);
}

int	main(int argc, char** argv)
{
	(void)argc;
	(void)argv;

	std::vector<Server>	servers;
	Server	server;
	server.port = 10;
	servers.push_back(server);
	server.port = 9999;
	servers.push_back(server);

	init_sockets(servers);

	try
	{
		while (true)
		{
			if (poll_servers(servers) <= 0)
				continue ;
			for (const Server& server : servers)
			{
				if (server.poll_fd->revents)
					handle_request(server);
			}
			// sleep(1);
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << (errno ? ": " + std::string(strerror(errno)) : "") << std::endl;
		exit(1);
	}
}
