#include "webserv.hpp"

in_addr	host_string_to_in_addr(const std::string& host)
{
	in_addr addr;
	addrinfo hints{};
	addrinfo* res = nullptr;

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(host.c_str(), nullptr, &hints, &res) != 0)
		throw std::runtime_error("Failed to resolve address");

	addr = ((sockaddr_in*)res->ai_addr)->sin_addr;

	freeaddrinfo(res);
	return (addr);
}

void	init_sockaddr(Server& server)
{
	server.sockaddr.sin_addr = host_string_to_in_addr(server.host);
	server.sockaddr.sin_family = AF_INET;
	server.sockaddr.sin_port = htons(server.port);
}

void	init_sockets(std::vector<Server>& servers)
{
	for (Server& server : servers)
	{
		server.socket = socket(PF_INET, SOCK_STREAM, 0);
		if (server.socket < 0)
			throw std::runtime_error("Failed to open socket");
		int opt = 1;
		if (setsockopt(server.socket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0)
			throw std::runtime_error("setsockopt failed");
		if (fcntl(server.socket, F_SETFL, O_NONBLOCK) < 0)
			throw std::runtime_error("fcntl failed");

		init_sockaddr(server);
		if (bind(server.socket, (sockaddr*)& server.sockaddr, sizeof(server.sockaddr)) < 0)
			throw std::runtime_error("Failed to bind to " + server.host + ':' + std::to_string(server.port) + ':' + std::string(strerror(errno)));
		if (listen(server.socket, 1024) < 0)
			throw std::runtime_error("Failed to listen on socket");
		std::cout << "init " << server.host << ':' << server.port << std::endl;
	}
}

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
	std::cout << std::filesystem::is_directory("/Users/bwerner/Documents/projects/rank05/webserv/github_webserv/html/test/index.htmlx") << std::endl;
	std::cout << argv[0] << "/html/" << std::endl;
	std::cout << std::filesystem::current_path().string() + "/html/" << std::endl << std::endl << std::endl;
	std::vector<Server> servers;

	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, sigint_handler);

	parser(servers, argc > 1 ? argv[1] : "webserver.conf");
	if (servers.empty())
		return EXIT_FAILURE;
	add_fallback_locations(servers); // TEMPORARY FUNCTION. should happen in parser.
	expand_relative_roots(servers); // TEMPORARY FUNCTION. should happen in parser.

	init_sockets(servers);

	// servers[0].conf.begin()->second.locations.begin()->second.autoindex = true; // TEMP

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
			std::cout << server.host << ':' << server.port << " has " << server.connections.size() << " connections | ";
		std::cout << std::endl;
	}
}
