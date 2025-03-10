#include "webserv.hpp"

bool isValidLocationKey(const std::string& key)
{
	static const std::set<std::string> validKeys =
	{
		"root",
		"autoindex",
		"index",
		"upload_dir",
		"cgi_extension",
		"allow_methods",
		"client_max_body_size"
	};
	return validKeys.find(key) != validKeys.end();
}

bool isValidServerKey(const std::string& key)
{
	static const std::set<std::string> validKeys =
	{
		"listen",
		"server_name",
		"root",
		"index",
		"client_max_body_size",
		"error_page",
		"autoindex"
	};
	return validKeys.find(key) != validKeys.end();
}

void expand_relative_roots(std::vector<Server>& servers)
{
	for (Server& s : servers)
	{
		for (ServerConfig& c : s.conf)
		{
			if (!c.root.empty() && c.root.front() != '/')
				c.root = std::filesystem::current_path().string() + '/' + c.root;
			std::cout << "ROOT---------------------------------------------------" << c.root << std::endl;
			for (LocationConfig& l : c.locations)
			{
				if (!l.root.empty() && l.root.front() != '/')
					l.root = std::filesystem::current_path().string() + '/' + l.root;
				std::cout << "ROOT---------------------------------------------------" << l.root << std::endl;
			}
		}
	}
}

bool validateHost(const std::vector<Server>& servers)
{
	bool isValidHost = true;

	for (const auto& server : servers)
	{
		for (const auto& config : server.conf)
		{
			if (config.host.s_addr == INADDR_ANY)
				continue;
			if (config.host_str != "0.0.0.0" && config.host_str != "127.0.0.1" && config.host_str != "localhost")
			{
				std::regex hostnameRegex("^[a-zA-Z0-9]([a-zA-Z0-9\\-\\.]{0,61}[a-zA-Z0-9])?$");
				if (!std::regex_match(config.host_str, hostnameRegex))
				{
					std::cerr 
						<< "Warnung: Möglicherweise ungültiger Host in Server '" 
						<< config.server_name << "': " << config.host_str << std::endl;
				}
			}
		}
	}
	return isValidHost;
}

bool validatePort(const std::vector<Server>& servers)
{
	bool isValidPort = true;

	for (size_t i = 0; i < servers.size(); i++)
	{
		const auto& server = servers[i];
		if (server.port < 1)
		{
			std::cerr << "Critical Error: Invalid port in server: " 
				<< server.port << std::endl;
			isValidPort = false;
		}
		if (server.port < 1024 && getuid() != 0)
		{
			std::cerr 
			<< "Critical Error: Port " 
			<< server.port 
			<< " Ports below 1024 require root privileges, but the program is not running as root!"
			<< std::endl;
			isValidPort = false;
		}
	}
	return isValidPort;
}

bool validateRoot(const std::vector<Server>& servers)
{
	bool isValid = true;

	for (const auto& server : servers)
	{
		for (const auto& config : server.conf)
		{
			if (!std::filesystem::exists(config.root) || !std::filesystem::is_directory(config.root))
			{
				std::cerr << "Warning: Root directory does not exist or is not a directory: " 
					<< config.root << " in server " << config.server_name << std::endl;
				isValid = false;
			}
		}
	}
	return isValid;
}

bool validateIndex(const std::vector<Server>& servers)
{
	bool isValid = true;

	for (const auto& server : servers)
	{
		for (const auto& config : server.conf)
		{
			if (!config.index.empty())
			{
				std::string indexPath = config.root + "/" + config.index;
				if (!std::filesystem::exists(indexPath))
				{
					std::cerr << "Warning: Index file does not exist: " 
						<< indexPath << " in server " << config.server_name << std::endl;
				}
			}
		}
	}
	return isValid;
}

bool validateErrorPage(const std::vector<Server>& servers)
{
	bool isValid = true;

	for (const auto& server : servers)
	{
		for (const auto& config : server.conf)
		{
			for (const auto& [errorCode, errorPage] : config.error_page)
			{
				std::string errorPagePath = config.root + "/" + errorPage;
				if (!std::filesystem::exists(errorPagePath))
				{
					std::cerr << "Warning: Error page file does not exist: " 
						<< errorPagePath << " for error code " << errorCode 
						<< " in server " << config.server_name << std::endl;
				}
			}
		}
	}
	return isValid;
}

bool validateClientMaxBodySize(const std::vector<Server>& servers)
{
	bool isValid = true;

	for (const auto& server : servers)
	{
		for (const auto& config : server.conf)
		{
			if (config.client_max_body_size > 1073741824)
			{
				std::cerr << "Warning: Very large client_max_body_size: " 
					<< config.client_max_body_size << " bytes in server " 
					<< config.server_name << std::endl;
			}
		}
	}
	return isValid;
}

bool validateServerName(const std::vector<Server>& servers)
{
	bool isValid = true;
	std::map<std::pair<in_addr_t, uint16_t>, std::set<std::string>> serverNamesByAddress;

	for (const auto& server : servers)
	{
		for (const auto& config : server.conf)
		{
			std::pair<in_addr_t, uint16_t> addrPort = {config.host.s_addr, config.port};
			
			if (!config.server_name.empty() && 
				serverNamesByAddress[addrPort].find(config.server_name) != serverNamesByAddress[addrPort].end())
			{
				std::cerr << "Error: Duplicate server_name: " 
					<< config.server_name << " for host:port " 
					<< config.host_str << ":" << config.port << std::endl;
				isValid = false;
			}
			
			serverNamesByAddress[addrPort].insert(config.server_name);
		}
	}
	return isValid;
}

bool validateAutoindex(const std::vector<Server>& servers)
{
	bool isValid = true;

	for (const auto& server : servers)
	{
		for (const auto& config : server.conf)
		{
			if (config.autoindex && (!std::filesystem::exists(config.root) || !std::filesystem::is_directory(config.root)))
			{
				std::cerr << "Warning: Autoindex is enabled but root directory does not exist: " 
					<< config.root << " in server " << config.server_name << std::endl;
			}
		}
	}
	return isValid;
}

LocationConfig	get_fallback_location(const ServerConfig& server_config)
{
	LocationConfig config;
	config.path = "/";
	config.root = server_config.root;
	config.allow_methods = std::set<std::string>({"GET", "POST", "DELETE"});
	config.autoindex = server_config.autoindex;
	config.index = server_config.index;
	config.client_max_body_size = server_config.client_max_body_size;
	config.error_page = server_config.error_page;

	return (config);
}

bool validateLocations(std::vector<Server>& servers)
{
	bool isValid = true;
	
	for (const auto& server : servers)
	{
		for (const auto& config : server.conf)
		{
			if (config.locations.empty())
			{
				std::cerr << "Warning: No locations defined in server " 
					<< config.server_name << std::endl;
			}
			std::map<std::string, int> pathCounts;
			for (const auto& loc : config.locations)
			{
				pathCounts[loc.path]++;
				if (pathCounts[loc.path] > 1)
				{
					std::cerr << "Warning: Duplicate location path: " 
						<< loc.path << " in server " << config.server_name << std::endl;
				}
			}
			if (pathCounts["/"] == 0)
			{
				std::cerr << "Warning: No fallback location '/' defined in server " 
					<< config.server_name << ". A default one will be added." << std::endl;
				for (Server& s : servers)
				{
					for (ServerConfig& c : s.conf)
					{
						bool has_fallback_location = false;
						for (LocationConfig& l : c.locations)
						{
							if (l.path == "/")
								has_fallback_location = true;
						}
						if (!has_fallback_location)
							c.locations.insert(c.locations.end(), get_fallback_location(c));
					}
				}
			}
		}
	}
	return isValid;
}

bool validateConfigurations(std::vector<Server>& servers) 
{
	if (servers.empty())
	{
		std::cerr << "Error: No valid server configurations found" << std::endl;
		return false;
	}

	expand_relative_roots(servers);

	bool hasCriticalErrors = false;
	bool hasWarnings = false;

	if (!validatePort(servers)) 
		hasCriticalErrors = true;

	if (!validateHost(servers) ||
		!validateRoot(servers) || 
		!validateIndex(servers) || 
		!validateErrorPage(servers) || 
		!validateClientMaxBodySize(servers) || 
		!validateServerName(servers) || 
		!validateAutoindex(servers) || 
		!validateLocations(servers)
		// !validateLocationPath(servers) || 
		// !validateLocationRoot(servers) || 
		// !validateLocationAllowMethods(servers) || 
		// !validateLocationAutoIndex(servers) || 
		// !validateLocationIndex(servers) || 
		// !validateLocationClientBodyTempPath(servers) || 
		// !validateLocationFastcgiParam(servers) || 
		// !validateLocationClientMaxBodySize(servers) || 
		// !validateLocationErrorPage(servers) ||
		)
	{
		hasWarnings = true;
	}

	if (hasCriticalErrors)
	{
		std::cerr << "Configuration validation completed with CRITICAL ERRORS." << std::endl;
		std::cerr << "Server will not start due to configuration issues." << std::endl;
		return false;
	}
	else if (hasWarnings)
	{
		std::cout << "Configuration validation completed with WARNINGS." << std::endl;
		std::cout << "Server will start, but some functionality may be limited." << std::endl;
		return true;
	}
	else
	{
		std::cout << "Configuration validation completed successfully." << std::endl;
		return true;
	}
}