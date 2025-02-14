#include "webserv.hpp"

int	main(int argc, char** argv)
{
	try
	{
		(void)argc;
		(void)argv;
		std::cout << "hello" << std::endl;
		int fd_socket = socket(PF_INET, SOCK_STREAM, 0);
		if (fd_socket < 0)
			throw std::runtime_error("Failed to open socket");

		sockaddr_in sockaddr;
		sockaddr.sin_family = AF_INET;
		sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		sockaddr.sin_port = htons(9999);

		if (bind(fd_socket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0)
			throw std::runtime_error("Failed to bind to port 9999");
		if (listen(fd_socket, 10) < 0)
			throw std::runtime_error("Failed to listen on socket");

		socklen_t addrlen = sizeof(sockaddr);
		while (true)
		{
			int fd_connection = accept(fd_socket, (struct sockaddr*)&sockaddr, &addrlen);
			if (fd_connection < 0)
				throw std::runtime_error("Failed to grab connection");

			char buffer[1024] = {0};
			(void)read(fd_connection, buffer, 1024);
			std::cout << "The message was: " << buffer;

			std::ifstream index("index.html");
			if (!index.is_open())
				std::cerr << "failed to open" << std::endl;
			std::string body;
			std::getline(index, body, '\0');
			if (index.fail())
				std::cerr << "failed to getline" << std::endl;
			std::string content_length = std::to_string(body.size()).c_str();
			(void)content_length;
			std::ostringstream header;
			header	<<	"HTTP/1.1 200 OK\n"
					<<	"Content-Type: text/html\n"
					<<	"Content-Length: " << body.size() << '\n'
					<<	"Connection: keep-alive\n"
					<<	"\n";
			std::string response = header.str() + body;
			send(fd_connection, response.c_str(), response.size(), 0);
			close(fd_connection);
		}

		close(fd_socket);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << (errno ? ": " + std::string(strerror(errno)) : "") << std::endl;
		exit(1);
	}
}
