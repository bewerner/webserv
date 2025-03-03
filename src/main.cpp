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
	// servers.push_back(servers[0]);
	// servers.erase(servers.begin());
	for (Server& server : servers)
	{
		server.socket = socket(PF_INET, SOCK_STREAM, 0);
		if (server.socket < 0)
			throw std::runtime_error("Failed to open socket");
		int opt = 1;
		if (setsockopt(server.socket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0)
			throw std::runtime_error("setsockopt failed");
		fcntl(server.socket, F_SETFL, O_NONBLOCK);

		init_sockaddr(server);
		if (bind(server.socket, (sockaddr*)& server.sockaddr, sizeof(server.sockaddr)) < 0)
			throw std::runtime_error("Failed to bind to " + server.host + ':' + std::to_string(server.port) + ':' + std::string(strerror(errno)));
		if (listen(server.socket, 1024) < 0)
			throw std::runtime_error("Failed to listen on socket");
		std::cout << "init " << server.host << ':' << server.port << std::endl;
	}
	// for (Server& server : servers)
	// {
	// 	if (server.socket != -1)
	// 		continue;
	// 	//server.init_socket() // maybe put this in Server constructor?
	// 	server.socket = socket(PF_INET, SOCK_STREAM, 0);
	// 	if (server.socket < 0)
	// 		throw std::runtime_error("Failed to open socket");
	// 	fcntl(server.socket, F_SETFL, O_NONBLOCK);

	// 	init_sockaddr(server);
	// 	if (bind(server.socket, (sockaddr*)& server.sockaddr, sizeof(server.sockaddr)) < 0)
	// 		throw std::runtime_error("Failed to bind to " + server.host + ':' + std::to_string(server.port) + ':' + std::string(strerror(errno)));
	// 	int opt = 1;
	// 	setsockopt(server.socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	// 	if (listen(server.socket, 1024) < 0)
	// 		throw std::runtime_error("Failed to listen on socket");
	// 	std::cout << "init " << server.host << ':' << server.port << std::endl;
	// }
}

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

	//--------------temporary for testing--------------------------
	// servers[0].port = 8080;
	// servers[1].port = 8081;
	// servers[2].port = 8082;
	// servers[0].request_timeout = std::chrono::seconds(100);
	// servers[1].request_timeout = std::chrono::seconds(100);
	// servers[2].request_timeout = std::chrono::seconds(100);
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
			std::cout << server.host << ':' << server.port << " has " << server.connections.size() << " connections | ";
		std::cout << std::endl;
	}
	// catch (const std::exception& e)
	// {
	// 	std::cerr << e.what() << (errno ? ": " + std::string(strerror(errno)) : "") << std::endl;
	// 	exit(1);
	// }
}
