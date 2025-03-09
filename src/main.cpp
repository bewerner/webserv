#include "webserv.hpp"

void sigint_handler(int signal)
{
	(void)signal;
	std::cout << "exit" << std::endl;
	// system("leaks webserv"); // debug
	exit(EXIT_SUCCESS);
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

LocationConfig	get_fallback_location(const ServerConfig& server_config)
{
	LocationConfig config;
	config.path = "/";
	config.root = server_config.root;
	config.allow_methods = std::set<std::string>({"GET", "POST", "DELETE"});
	config.autoindex = server_config.autoindex;
	config.index = server_config.index;
	config.client_max_body_size = server_config.client_max_body_size;
	config.error_page = server_config.error_page;

	return (config);
}

void	add_fallback_locations(std::vector<Server>& servers)
{
	for (Server& s : servers)
	{
		for (ServerConfig& c : s.conf)
		{
			bool has_fallback_location = false;
			for (LocationConfig& l : c.locations)
			{
				if (l.path == "/")
					has_fallback_location = true;
			}
			if (!has_fallback_location)
				c.locations.insert(c.locations.end(), get_fallback_location(c));
		}
	}
}

void	expand_relative_roots(std::vector<Server>& servers)
{
	for (Server& s : servers)
	{
		for (ServerConfig& c : s.conf)
		{
			if (c.root.front() != '/')
				c.root = std::filesystem::current_path().string() + '/' + c.root;
			std::cout << "ROOT---------------------------------------------------" << c.root << std::endl;
			for (LocationConfig& l : c.locations)
			{
				if (l.root.front() != '/')
					l.root = std::filesystem::current_path().string() + '/' + l.root;
				std::cout << "ROOT---------------------------------------------------" << l.root << std::endl;
			}
		}
	}
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
				if (*connection.revents & POLLIN)
					connection.receive();
				else if (*connection.revents & POLLOUT)
					connection.respond();
				else if (*connection.revents & (POLLHUP | POLLERR))
				{
					if (*connection.revents & POLLHUP) // debug
						std::cout << "POLLHUP" << std::endl;
					if (*connection.revents & POLLERR) // debug
						std::cout << "POLLERR" << std::endl;
					connection.close = true;
				}
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
}