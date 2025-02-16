#include "webserv.hpp"

int main(int argc, char** argv)
{
	std::vector<Server> servers;
	std::string path(argc > 1 ? argv[1] : "webserver.conf");

	try {
		parser(servers, path);
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
