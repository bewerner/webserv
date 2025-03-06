#include "webserv.hpp"

void init_sockaddr(Server& server)
{
	server.sockaddr.sin_addr = server.host;
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
			throw std::runtime_error("Failed to bind to " + server.host_str + ':' + std::to_string(server.port) + ':' + std::string(strerror(errno)));
		if (listen(server.socket, 1024) < 0)
			throw std::runtime_error("Failed to listen on socket");
		std::cout << "init " << server.host_str << ':' << server.port << std::endl;
	}
}