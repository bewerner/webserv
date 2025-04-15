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

std::string removeSpaces(const std::string& str)
{
	const std::string whitespace = " \t\n\r\f\v";
	size_t start = str.find_first_not_of(whitespace);
	if (start == std::string::npos)
		return "";
	size_t end = str.find_last_not_of(whitespace);
	return str.substr(start, end - start + 1);
}

std::string removeComments(const std::string& str)
{
	std::string result;
	std::istringstream stream(str);
	std::string line;

	while (std::getline(stream, line))
	{
		size_t commentPos = line.find('#');
		if (commentPos != std::string::npos)
			line = line.substr(0, commentPos);
		if (!removeSpaces(line).empty())
			result += line + "\n";
	}
	return result;
}

void printData(const std::vector<Server>& servers)
{
	std::cout << "Number of servers: " << servers.size() << '\n';
	size_t serverNum = 1;

	for (const auto& server : servers)
	{
		std::cout << std::endl;
		std::string host_ip = inet_ntoa(server.host);
		std::cout << "Server " << serverNum++ << " (" << host_ip << ":" << server.port << "):\n";
		if (!server.conf.empty())
		{
			std::string defaultName = server.conf[0].server_name;
			if (defaultName.empty())
				defaultName = "(NONAME)" + host_ip + ":" + std::to_string(server.port);
			std::cout << "Default Server Name: " << defaultName << "\n";
		}
		else
			std::cout << "Default Server Name: NAMELESS\n";
		
		for (const auto& config : server.conf)
		{
			std::string configName = config.server_name;
			if (configName.empty())
				configName = "(NONAME)" + host_ip + ":" + std::to_string(server.port);
				
			std::cout << "Config Name: " << configName << "\n";
			std::cout << "Host: " << inet_ntoa(config.host) << " (Original String: " << config.host_str << ")\n";
			std::cout << "Port: " << config.port << "\n";
			if (!config.root.empty())
				std::cout << "Root: " << config.root << "\n";
			if (!config.index.empty())
				std::cout << "Index: " << config.index << "\n";
			std::cout << "Auto Index: " << (config.autoindex ? "enabled" : "disabled") << "\n";
			std::cout << "Max Client Body Size: " << config.client_max_body_size << "\n";
			std::cout << "Server Name: ";
			if (config.server_name.empty())
				std::cout << "NONE";
			else
				std::cout << config.server_name << " ";
			std::cout << "\n";
			std::cout << "Error Pages: \n";
			for (const auto& [errorCode, errorPage] : config.error_page)
				std::cout << "  " << errorCode << ": " << errorPage << "\n";
			std::cout << "Locations: \n";
			size_t locNum = 1;
			for (const auto& loc : config.locations)
			{
				std::cout << locNum++ << ":\n";
				std::cout << "  Path: " << loc.path << "\n";
				if (!loc.root.empty())
					std::cout << "  Document Root: " << loc.root << "\n";
				if (!loc.alias.empty())
					std::cout << "  Alias: " << loc.alias << "\n";
				std::cout << "  Auto Index: " << (loc.autoindex ? "enabled" : "disabled") << "\n";
				std::cout << "  Default File: " << loc.index << "\n";
				std::cout << "  CGI: " << (loc.cgi ? "enabled" : "disabled") << "\n";
				std::cout << "  Max Client Body Size: " << loc.client_max_body_size << "\n";
				std::cout << "  DAV Methods: ";
				for (const auto& method : loc.dav_methods)
					std::cout << method << " ";
				std::cout << "\n";
				
				std::cout << "  Error Pages: \n";
				for (const auto& [errorCode, errorPage] : loc.error_page)
					std::cout << "    " << errorCode << ": " << errorPage << "\n";
			}
			std::cout << "----------------------------------------\n";
		}
	}
	std::cout << "\n=== IP:PORT TO DEFAULT SERVER MAPPING ===\n";
	for (const auto& server : servers)
	{
		std::string host_ip = inet_ntoa(server.host);
		std::string ipPort = host_ip + ":" + std::to_string(server.port);
		std::string defaultName = "NAMELESS";
		if (!server.conf.empty())
		{
			defaultName = server.conf[0].server_name;
			if (defaultName.empty())
				defaultName = "(NONAME)" + host_ip + ":" + std::to_string(server.port);
		}
		std::cout << std::left << std::setw(30) << ipPort 
					<< " -> Default Server: " << defaultName << "\n";
		std::cout << "   Available servers: ";
		bool firstPrinted = false;
		for (const auto& config : server.conf)
		{
			if (firstPrinted)
				std::cout << ", ";
			std::string displayName = config.server_name;
			if (displayName.empty())
				displayName = "(NONAME)" + host_ip + ":" + std::to_string(server.port);
			if (&config == &server.conf[0])
				std::cout << displayName << " (DEFAULT)";
			else
				std::cout << displayName;
				
			firstPrinted = true;
		}
		std::cout << ", \n\n";
	}
}