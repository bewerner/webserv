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
	
	size_t size = 0;
	for (Server& server : servers)
	{
		size++;
		for (Connection& connection : server.connections)
		{
			size++;
			CGI& cgi = connection.response.cgi;
			if (cgi.pid >= 0 && !cgi.fail)
			{
				if (cgi.pipe_into_cgi[1] >= 0)
					size++;
				if (cgi.pipe_from_cgi[0] >= 0)
					size++;
			}
		}
	}
	fds.reserve(size);

	for (Server& server : servers)
	{
		fds.emplace_back(pollfd{.fd = server.socket, .events = POLLIN, .revents = 0});
		server.revents = &fds.back().revents;
		for (Connection& connection : server.connections)
		{
			connection.revents = nullptr;
			// if ((connection.buffer.empty() && connection.events == POLLIN) || connection.events == POLLOUT)
			{
				// std::cout << "x" << std::endl;
				fds.emplace_back(pollfd{.fd = connection.fd, .events = connection.events, .revents = 0});
				connection.revents = &fds.back().revents;
			}
			CGI& cgi = connection.response.cgi;
			cgi.revents_write_into_cgi = nullptr;
			cgi.revents_read_from_cgi = nullptr;
			if (cgi.pid >= 0 && !cgi.fail)
			{
				if (cgi.pipe_into_cgi[1] >= 0)
				{
					// if (cgi.is_running())
					// {
						std::cout << "pidpoll1" << std::endl;
						fds.emplace_back(pollfd{.fd = cgi.pipe_into_cgi[1], .events = POLLOUT, .revents = 0});
						cgi.revents_write_into_cgi = &fds.back().revents;
					// }
					// else
					// 	cgi.fail = true;
				}
				else if (cgi.pipe_from_cgi[0] >= 0)
				{
					std::cout << "pidpoll2" << std::endl;
					fds.emplace_back(pollfd{.fd = cgi.pipe_from_cgi[0], .events = POLLIN, .revents = 0});
					cgi.revents_read_from_cgi = &fds.back().revents;
				}
			}
		}
	}

	return (poll(fds.data(), fds.size(), 1000));
}

int	main(int argc, char** argv)
{
	std::vector<Server> servers;

	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, sigint_handler);

	try 
	{
		parser(servers, argc > 1 ? argv[1] : "webserver.conf");
	}
	catch (const std::exception& e) 
	{
		std::cerr << "Parser error: " << e.what() << std::endl;
		return (EXIT_FAILURE);
	}
	if (servers.empty())
		return (EXIT_FAILURE);

	while (true)
	{
		std::cout << "poll: " << poll_servers(servers) << std::endl;
		for (Server& server : servers)
		{
			for (Connection& connection : server.connections)
			{
				try
				{
					const CGI& cgi = connection.response.cgi;
					if (connection.pollin() || cgi.pollout())
						connection.receive();
					else if (connection.pollout() || cgi.pollin())
						connection.respond();
					else if (connection.pollerr() || connection.pollhup())
						connection.close = true;
				}
				catch (const std::exception& e)
				{
					connection.handle_exception(e);
				}
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
