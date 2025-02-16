#include "webserv.hpp"

void printData(std::vector<Server> servers) {
	std::cout << "Number of servers: " << servers.size() << '\n';

	for (const auto& server : servers)
	{
		std::cout << std::endl;
		static size_t serverNum = 1;
		std::cout << "Server " << serverNum++ << ":\n";
		for (const auto& [key, value] : server.config)
			std::cout << key << ": " << value << '\n';
		std::cout << "Locations:\n";
		size_t locNum = 1;
		for (const auto& loc : server.locations)
		{
			std::cout << locNum++ << ":\n";

			for (const auto& [key, value] : loc.location_config)
				std::cout << key << ": " << value << '\n';
		}
	}
}


int main(int argc, char** argv)
{
	std::vector<Server> servers;
	std::string path(argc > 1 ? argv[1] : "webserver.conf");

	try {
		parser(servers, path);
		printData(servers);
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
