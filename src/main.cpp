#include "webserv.hpp"

int	main(int argc, char** argv)
{
	try
	{
		(void)argc;
		(void)argv;
		std::cout << "hello" << std::endl;
		int socket_fd = socket(PF_INET, SOCK_STREAM, 0);
		if (socket_fd < 0)
			throw std::runtime_error("Failed to open socket");

		sockaddr_in sockaddr;
		sockaddr.sin_family = AF_INET;
		sockaddr.sin_addr.s_addr = INADDR_ANY;
		sockaddr.sin_port = htons(9999);

		if (bind(socket_fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0)
			throw std::runtime_error("Failed to bind to port 9999");
		if (listen(socket_fd, 10) < 0)
			throw std::runtime_error("Failed to listen on socket");

		socklen_t addrlen = sizeof(sockaddr);
		int connection = accept(socket_fd, (struct sockaddr*)&sockaddr, &addrlen);
		if (connection < 0)
			throw std::runtime_error("Failed to grab connection");

		char buffer[100];
		(void)read(connection, buffer, 100);
		std::cout << "The message was: " << buffer;

		std::ifstream index("index.html");
		if (!index.is_open())
			std::cerr << "failed to open" << std::endl;
		std::string str;
		std::getline(index, str, '\0');
		if (index.fail())
			std::cerr << "failed to getline" << std::endl;
		send(connection, str.c_str(), str.size(), 0);

		close(connection);
		close(socket_fd);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << (errno ? ": " + std::string(strerror(errno)) : "") << std::endl;
		exit(1);
	}
}
