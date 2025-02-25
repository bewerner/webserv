#include "webserv.hpp"

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

void sigint_handler(int signal)
{
	(void)signal;
	std::cout << "exit" << std::endl;
	exit(0);
}

int	poll_servers(std::vector<Server>& servers)
{
	static std::vector<pollfd> fds;
	fds.clear();

	size_t size = servers.size();
	for (Server& server : servers)
		size += server.connections.size();
	fds.reserve(size);

	for (Server& server : servers)
	{
		fds.emplace_back(pollfd{.fd = server.socket, .events = POLLIN, .revents = 0});
		server.revents = &fds.back().revents;
		for (Connection& connection : server.connections)
		{
			fds.emplace_back(pollfd{.fd = connection.fd, .events = connection.events, .revents = 0});
			connection.revents = &fds.back().revents;
		}
	}

	return (poll(fds.data(), fds.size(), 1000));
}

int	main(int argc, char** argv)
{
	std::vector<Server> servers;

	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, sigint_handler);

	parser(servers, argc > 1 ? argv[1] : "webserver.conf");

	//--------------temporary for testing--------------------------
	servers[0].port = 8080;
	servers[1].port = 8081;
	servers[2].port = 8082;
	servers[0].request_timeout = std::chrono::seconds(100);
	servers[1].request_timeout = std::chrono::seconds(100);
	servers[2].request_timeout = std::chrono::seconds(100);
	//-------------------------------------------------------------

	init_sockets(servers);

	while (true)
	{
		poll_servers(servers);
		for (Server& server : servers)
		{
			for (Connection& connection : server.connections)
			{
				if (*connection.revents & (POLLHUP | POLLERR))
					connection.close = true;
				else if (*connection.revents & POLLIN)
					connection.receive();
				else if (*connection.revents & POLLOUT)
					connection.respond();
				else if (*connection.revents)
					throw std::logic_error("this should never happen. investigate"); // temp for debugging
			}
			if (*server.revents)
				server.accept_connection();
			server.clean_connections(); //close marked connections and timed out connections
		}

		// debug
		for (Server& server : servers)
			std::cout << server.port << " has " << server.connections.size() << " connections | ";
		std::cout << std::endl;
	}
	// catch (const std::exception& e)
	// {
	// 	std::cerr << e.what() << (errno ? ": " + std::string(strerror(errno)) : "") << std::endl;
	// 	exit(1);
	// }
}
