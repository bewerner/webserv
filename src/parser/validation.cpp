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
		"error_page"
	};
	return validKeys.find(key) != validKeys.end();
}

bool validateHost(const std::vector<Server>& servers)
{
	bool isValidHost = true;

	for (const auto& server : servers)
	{
		for (const auto& config : server.conf)
		{
			if (config.host == "0.0.0.0")
				continue;
			try
			{
				host_string_to_in_addr(config.host);
				continue;
			}
			catch (const std::runtime_error&)
			{
				std::regex hostnameRegex("^[a-zA-Z0-9]([a-zA-Z0-9\\-\\.]{0,61}[a-zA-Z0-9])?$");
				if (std::regex_match(config.host, hostnameRegex))
					continue;
					
				std::cerr 
					<< "Error: Invalid host in server '" << config.server_name << "': " << config.host << std::endl;
				isValidHost = false;
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
			std::cerr << "Error: Invalid port in server: " 
				<< server.port << std::endl;
			isValidPort = false;
		}
		if (server.port < 1024 && getuid() != 0)
		{
			std::cerr 
			<< "Error: Port " 
			<< server.port 
			<< " Ports below 1024 require root privileges, but the program is not running as root!"
			<< std::endl;
			isValidPort = false;
		}
	}
	return isValidPort;
}

bool validateConfigurations(const std::vector<Server>& servers) 
{
	if (servers.empty())
	{
		std::cerr << "Error: No valid server configurations found" << std::endl;
		return false;
	}

	bool isValid = true;

	isValid = validateHost(servers) && isValid;
	isValid = validatePort(servers) && isValid;
	// isValid = validateRoot(servers) && isValid;
	// isValid = validateIndex(servers) && isValid;
	// isValid = validateErrorPage(servers) && isValid;
	// isValid = validateClientMaxBodySize(servers) && isValid;
	// isValid = validateServerName(servers) && isValid;
	// isValid = validateLocations(servers) && isValid;

	// isValid = validatePath(servers) && isValid;
	// isValid = validateRoot(servers) && isValid;
	// isValid = validateAllowMethods(servers) && isValid;
	// isValid = validateAutoIndex(servers) && isValid;
	// isValid = validateIndex(servers) && isValid;
	// isValid = validateClientBodyTempPath(servers) && isValid;
	// isValid = validateFastcgiParam(servers) && isValid;
	// isValid = validateLocationClientMaxBodySize(servers) && isValid;

	return isValid;
}