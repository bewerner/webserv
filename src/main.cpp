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

	// size_t size = servers.size();
	// for (Server& server : servers)
	// 	size += server.connections.size();
	
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
			fds.emplace_back(pollfd{.fd = connection.fd, .events = connection.events, .revents = 0});
			connection.revents = &fds.back().revents;
			CGI& cgi = connection.response.cgi;
			if (cgi.pid >= 0 && !cgi.fail)
			{
				if (cgi.pipe_into_cgi[1] >= 0)
				{
					fds.emplace_back(pollfd{.fd = cgi.pipe_into_cgi[1], .events = POLLOUT, .revents = 0});
					cgi.revents_write_into_cgi = &fds.back().revents;
				}
				else
					cgi.revents_write_into_cgi = nullptr;
				if (cgi.pipe_from_cgi[0] >= 0)
				{
					fds.emplace_back(pollfd{.fd = cgi.pipe_from_cgi[0], .events = POLLIN, .revents = 0});
					cgi.revents_read_from_cgi = &fds.back().revents;
				}
				else
					cgi.revents_read_from_cgi = nullptr;
			}
			else
			{
				cgi.revents_write_into_cgi = nullptr;
				cgi.revents_read_from_cgi = nullptr;
			}
		}
	}

	return (poll(fds.data(), fds.size(), 1000));
}

int	main(int argc, char** argv, char** envp)
{
	try // TEMPORARY TRY-CATCH BLOCK ONLY FOR TESTER PERFORMANCE
	{
		std::vector<Server> servers;

		signal(SIGPIPE, SIG_IGN);
		signal(SIGINT, sigint_handler);

		parser(servers, argc > 1 ? argv[1] : "webserver.conf");
		if (servers.empty())
			return EXIT_FAILURE;
		for (Server& s : servers)
		{
			s.envp = envp;
		}

		while (true)
		{
			poll_servers(servers);
			for (Server& server : servers)
			{
				for (Connection& connection : server.connections)
				{
					const CGI& cgi = connection.response.cgi;
					if (*connection.revents & POLLIN || cgi.pollout())
						connection.receive();
					else if (*connection.revents & POLLOUT || cgi.pollin())
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
			// std::cout << "| ";
			// for (Server& server : servers)
			// 	std::cout << inet_ntoa(server.host) << ':' << server.port << " has " << server.connections.size() << " connections | ";
			// std::cout << std::endl;
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "UNCAUGHT EXCEPTION: " << e.what() << std::endl;
		exit(1);
	}
}
