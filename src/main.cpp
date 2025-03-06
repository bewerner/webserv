#include "webserv.hpp"

void sigint_handler(int signal)
{
	(void)signal;
	std::cout << "exit" << std::endl;
	// system("leaks webserv");
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
	if (servers.empty())
		return EXIT_FAILURE;
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
		std::cout << "| ";
		for (Server& server : servers)
			std::cout << inet_ntoa(server.host) << ':' << server.port << " has " << server.connections.size() << " connections | ";
		std::cout << std::endl;
	}
	// catch (const std::exception& e)
	// {
	// 	std::cerr << e.what() << (errno ? ": " + std::string(strerror(errno)) : "") << std::endl;
	// 	exit(1);
	// }
}