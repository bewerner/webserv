#include "webserv.hpp"

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
		std::cout << "Server " << serverNum++ << " (" << server.host << ":" << server.port << "):\n";
		std::cout << "Default Server Name: " << (server.default_server_name.empty() ? "NAMELESS" : server.default_server_name) << "\n";
		
		for (const auto& [name, config] : server.conf)
		{
			std::cout << "Config Name: " << (name.empty() ? "NAMELESS" : name) << "\n";
			std::cout << "Host: " << config.host << "\n";
			std::cout << "Port: " << config.port << "\n";
			if (!config.root.empty())
				std::cout << "Root: " << config.root << "\n";
			if (!config.index.empty())
				std::cout << "Index: " << config.index << "\n";
			std::cout << "Max Client Body Size: " << config.client_max_body_size << "\n";
			std::cout << "Server Name: ";
			if (config.server_name.empty())
				std::cout << "NONE";
			else
				for (const auto& serverName : config.server_name)
					std::cout << serverName << " ";
			std::cout << "\n";
			std::cout << "Error Pages: \n";
			for (const auto& [errorCode, errorPage] : config.error_page)
				std::cout << "  " << errorCode << ": " << errorPage << "\n";
			std::cout << "Locations: \n";
			size_t locNum = 1;
			for (const auto& [path, loc] : config.locations)
			{
				std::cout << locNum++ << ":\n";
				std::cout << "  Path: " << loc.path << "\n";
				std::cout << "  Document Root: " << loc.root << "\n";
				std::cout << "  Auto Index: " << (loc.autoindex ? "enabled" : "disabled") << "\n";
				std::cout << "  Default File: " << loc.index << "\n";
				std::cout << "  Upload Directory: " << loc.client_body_temp_path << "\n";
				std::cout << "  CGI Handler Extension: " << loc.fastcgi_param << "\n";
				std::cout << "  Allowed Methods: ";
				for (const auto& method : loc.allow_methods)
					std::cout << method << " ";
				std::cout << "\n";
			}
			std::cout << "----------------------------------------\n";
		}
	}
	
	std::cout << "\n=== IP:PORT TO DEFAULT SERVER MAPPING ===\n";
	for (const auto& server : servers)
	{
		std::string ipPort = server.host + ":" + std::to_string(server.port);
		std::string defaultName = server.default_server_name.empty() ? "NAMELESS" : server.default_server_name;
		
		std::cout << std::left << std::setw(30) << ipPort 
				  << " -> Default Server: " << defaultName << "\n";
		std::cout << "   Available servers: ";
		for (const auto& [name, _] : server.conf)
		{
			std::string displayName = name.empty() ? "NAMELESS" : name;
			std::cout << displayName;
			if (name == server.default_server_name)
				std::cout << " (DEFAULT)";
			std::cout << ", ";
		}
		std::cout << "\n\n";
	}
}