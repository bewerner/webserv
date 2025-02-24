#include "webserv.hpp"

void	Server::accept_connection(void)
{
	connections.emplace_back(this); // constructs a new connection in place
}

void	Server::clean_connections(void)
{
	auto now = std::chrono::steady_clock::now();
	for (auto it = connections.begin(); it != connections.end();)
	{
		if ((*it).close || (*it).timeout <= now)
			it = connections.erase(it);
		else
			it++;
	}
}
