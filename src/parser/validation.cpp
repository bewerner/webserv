#include "webserv.hpp"

bool isValidLocationKey(const std::string& key)
{
	static const std::set<std::string> validKeys =
	{
		"root",
		"alias",
		"autoindex",
		"index",
		"upload_dir",
		"cgi",
		"dav_methods",
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
			{
				try {
					c.root = std::filesystem::current_path().string() + '/' + c.root;
				} catch (const std::exception& e) {
					std::cerr << "Error expanding relative path for root: " << e.what() << std::endl;
				}
			}
			for (LocationConfig& l : c.locations)
			{
				if (!l.root.empty() && l.root.front() != '/')
				{
					try {
						l.root = std::filesystem::current_path().string() + '/' + l.root;
					} catch (const std::exception& e) {
						std::cerr << "Error expanding relative path for location root: " << e.what() << std::endl;
					}
				}
				if (!l.alias.empty() && l.alias.front() != '/')
				{
					try {
						l.alias = std::filesystem::current_path().string() + '/' + l.alias;
					} catch (const std::exception& e) {
						std::cerr << "Error expanding relative path for location alias: " << e.what() << std::endl;
					}
				}
			}
		}
	}
}

void validateHost(const std::vector<Server>& servers)
{
	for (const auto& server : servers)
	{
		for (const auto& config : server.conf)
		{
			if (config.host.s_addr == INADDR_ANY)
				continue;
				
			if (config.host_str == "0.0.0.0" || config.host_str == "127.0.0.1" || config.host_str == "localhost")
				continue;
				
			struct sockaddr_in sa;
			int result = inet_pton(AF_INET, config.host_str.c_str(), &(sa.sin_addr));
			if (result == 1)
				continue;
				
			std::regex hostnameRegex("^[a-zA-Z0-9]([a-zA-Z0-9\\-\\.]{0,61}[a-zA-Z0-9])?$");
			if (!std::regex_match(config.host_str, hostnameRegex))
			{
				std::cerr << "Warning: Potentially invalid host in server '" 
					<< config.server_name << "': " << config.host_str << std::endl;
			}
		}
	}
}

void validatePort(const std::vector<Server>& servers)
{
	for (size_t i = 0; i < servers.size(); i++)
	{
		const auto& server = servers[i];
		if (server.port < 1)
		{
			throw std::runtime_error("Critical Error: Invalid port in server: " 
				+ std::to_string(server.port));
		}
		
		for (size_t j = i + 1; j < servers.size(); j++) {
			if (server.host.s_addr == servers[j].host.s_addr && server.port == servers[j].port) {
				std::cout << "Warning: Multiple servers listening on "
					<< inet_ntoa(server.host) << ":" << server.port << std::endl;
			}
		}
	}
}

void validateRoot(const std::vector<Server>& servers)
{
	for (const auto& server : servers)
	{
		for (const auto& config : server.conf)
		{
			if (!config.root.empty())
			{
				try {
					if (!std::filesystem::exists(config.root))
					{
						std::cerr << "Warning: Root directory does not exist: " 
							<< config.root << " in server " << (config.server_name.empty() ? "[empty]" : config.server_name) << std::endl;
					}
					else if (!std::filesystem::is_directory(config.root))
					{
						std::cerr << "Warning: Root is not a directory: " 
							<< config.root << " in server " << (config.server_name.empty() ? "[empty]" : config.server_name) << std::endl;
					}
				} catch (const std::exception& e) {
					std::cerr << "Error checking root directory: " << e.what() << std::endl;
				}
			}
			
			for (const auto& loc : config.locations)
			{
				if (!loc.root.empty())
				{
					try {
						if (!std::filesystem::exists(loc.root))
						{
							std::cerr << "Warning: Location root directory does not exist: " 
								<< loc.root << " in location " << loc.path << std::endl;
						}
						else if (!std::filesystem::is_directory(loc.root))
						{
							std::cerr << "Warning: Location root is not a directory: " 
								<< loc.root << " in location " << loc.path << std::endl;
						}
					} catch (const std::exception& e) {
						std::cerr << "Error checking location root directory: " << e.what() << std::endl;
					}
				}
				
				if (!loc.alias.empty())
				{
					try {
						if (!std::filesystem::exists(loc.alias))
						{
							std::cerr << "Warning: Location alias directory does not exist: " 
								<< loc.alias << " in location " << loc.path << std::endl;
						}
						else if (!std::filesystem::is_directory(loc.alias))
						{
							std::cerr << "Warning: Location alias is not a directory: " 
								<< loc.alias << " in location " << loc.path << std::endl;
						}
					} catch (const std::exception& e) {
						std::cerr << "Error checking location alias directory: " << e.what() << std::endl;
					}
				}
			}
		}
	}
}

void validateRootAlias(const std::vector<Server>& servers)
{
	for (const auto& server : servers)
	{
		for (const auto& config : server.conf)
		{
			for (const auto& location : config.locations)
			{
				if (!location.root.empty() && !location.alias.empty())
				{
					throw std::runtime_error("Critical Error: Both 'root' and 'alias' directives are specified in location '" 
						+ location.path + "' in server " + (config.server_name.empty() ? "[empty]" : config.server_name));
				}
			}
		}
	}
}

LocationConfig get_fallback_location(const ServerConfig& server_config)
{
	LocationConfig config;
	config.path = "/";
	config.root = server_config.root;
	config.dav_methods = std::set<std::string>({"GET", "POST", "DELETE"});
	config.autoindex = server_config.autoindex;
	config.index = server_config.index;
	config.client_max_body_size = server_config.client_max_body_size;
	config.error_page = server_config.error_page;

	return (config);
}

void validateLocations(std::vector<Server>& servers)
{
	for (Server& s : servers)
	{
		for (ServerConfig& c : s.conf)
		{
			if (c.locations.empty())
			{
				std::cerr << "Warning: No locations defined in server " 
					<< (c.server_name.empty() ? "[empty]" : c.server_name) << ". Adding default location." << std::endl;
				
				c.locations.push_back(get_fallback_location(c));
			}
			else
			{
				std::map<std::string, int> pathCounts;
				for (const auto& loc : c.locations)
				{
					pathCounts[loc.path]++;
					if (pathCounts[loc.path] > 1)
					{
						std::cerr << "Warning: Duplicate location path: " 
							<< loc.path << " in server " << (c.server_name.empty() ? "[empty]" : c.server_name) << std::endl;
					}
					
					if (loc.path.empty() || loc.path[0] != '/')
					{
						std::cerr << "Warning: Location path doesn't start with a slash: " 
							<< loc.path << " in server " << (c.server_name.empty() ? "[empty]" : c.server_name) << std::endl;
					}
				}
				
				if (pathCounts["/"] == 0)
				{
					std::cerr << "Warning: No fallback location '/' defined in server " 
						<< (c.server_name.empty() ? "[empty]" : c.server_name) << ". A default one will be added." << std::endl;
					
					c.locations.push_back(get_fallback_location(c));
				}
			}
		}
	}
}

void validateIndex(const std::vector<Server>& servers)
{
    for (const auto& server : servers)
	{
		for (const auto& config : server.conf)
		{
			if (!config.index.empty())
			{
				try {
					std::string indexPath = config.root + "/" + config.index;
					if (!std::filesystem::exists(indexPath))
					{
						std::cerr << "Warning: Index file does not exist: " 
							<< indexPath << " in server " << (config.server_name.empty() ? "[empty]" : config.server_name) << std::endl;
					}
				} catch (const std::exception& e) {
					std::cerr << "Error checking index file: " << e.what() << std::endl;
				}
			}
			
			for (const auto& loc : config.locations)
			{
				if (!loc.index.empty() && !loc.root.empty())
				{
					try {
						std::string indexPath = loc.root + "/" + loc.index;
						if (!std::filesystem::exists(indexPath))
						{
							std::cerr << "Warning: Location index file does not exist: " 
								<< indexPath << " in location " << loc.path << std::endl;
						}
					} catch (const std::exception& e) {
						std::cerr << "Error checking location index file: " << e.what() << std::endl;
					}
				}
				else if (!loc.index.empty() && !loc.alias.empty())
				{
					try {
						std::string indexPath = loc.alias + "/" + loc.index;
						if (!std::filesystem::exists(indexPath))
						{
							std::cerr << "Warning: Location index file with alias does not exist: " 
								<< indexPath << " in location " << loc.path << std::endl;
						}
					} catch (const std::exception& e) {
						std::cerr << "Error checking location index file with alias: " << e.what() << std::endl;
					}
				}
			}
		}
	}
}

void validateUploadDir(const std::vector<Server>& servers)
{
	for (const auto& server : servers)
	{
		for (const auto& config : server.conf)
		{
			for (const auto& loc : config.locations)
			{
				if (!loc.client_body_temp_path.empty())
				{
					try {
						if (!std::filesystem::exists(loc.client_body_temp_path))
						{
							std::cerr << "Warning: Upload directory does not exist: " 
								<< loc.client_body_temp_path << " in location " << loc.path << std::endl;
							
							try {
								if (std::filesystem::create_directories(loc.client_body_temp_path))
								{
									std::cout << "Created upload directory: " << loc.client_body_temp_path << std::endl;
								}
								else
								{
									std::cerr << "Failed to create upload directory: " << loc.client_body_temp_path << std::endl;
								}
							} catch (const std::exception& e) {
								std::cerr << "Error creating upload directory: " << e.what() << std::endl;
							}
						}
						else if (!std::filesystem::is_directory(loc.client_body_temp_path))
						{
							std::cerr << "Error: Upload path is not a directory: " 
								<< loc.client_body_temp_path << " in location " << loc.path << std::endl;
						}
						else
						{
							std::string testFile = loc.client_body_temp_path + "/.webserv_write_test";
							std::ofstream test(testFile);
							if (!test.is_open())
							{
								std::cerr << "Warning: No write permission to upload directory: " 
									<< loc.client_body_temp_path << " in location " << loc.path << std::endl;
							}
							else
							{
								test.close();
								std::filesystem::remove(testFile);
							}
						}
					} catch (const std::exception& e) {
						std::cerr << "Error checking upload directory: " << e.what() << std::endl;
					}
				}
			}
		}
	}
}

void validateDavMethods(std::vector<Server>& servers)
{
	static const std::set<std::string> validMethods = {
		"GET", "POST", "DELETE"
	};

	for (const auto& server : servers)
	{
		for (const auto& config : server.conf)
		{
			for (const auto& loc : config.locations)
			{
				for (const auto& method : loc.dav_methods)
				{
					if (validMethods.find(method) == validMethods.end())
					{
						std::cerr << "Warning: Invalid HTTP method in dav_methods: " 
							<< method << " in location " << loc.path << std::endl;
					}
				}
				
				if (loc.dav_methods.find("GET") == loc.dav_methods.end())
				{
					std::cerr << "Warning: GET method not allowed in location " 
						<< loc.path << ". This might cause issues with browsers." << std::endl;
				}
				
				if (loc.dav_methods.empty())
				{
					std::cerr << "Warning: No HTTP methods allowed in location " 
						<< loc.path << ". Adding default methods (GET)." << std::endl;
					
					for (Server& s : servers)
					{
						for (ServerConfig& c : s.conf)
						{
							for (LocationConfig& l : c.locations)
							{
								if (l.path == loc.path && l.dav_methods.empty())
								{
									l.dav_methods.insert("GET");
								}
							}
						}
					}
				}
			}
		}
	}
}

void validateClientMaxBodySize(const std::vector<Server>& servers)
{
	for (const auto& server : servers)
	{
		for (const auto& config : server.conf)
		{
			if (config.client_max_body_size > 0)
			{
				if (config.client_max_body_size > 1073741824)
				{
					std::cerr << "Warning: Very large client_max_body_size: " 
						<< config.client_max_body_size << " bytes in server " 
						<< (config.server_name.empty() ? "[empty]" : config.server_name) 
						<< ". This might cause memory issues." << std::endl;
				}
			}
			for (const auto& loc : config.locations)
			{
				if (loc.client_max_body_size > 0)
				{
					if (loc.client_max_body_size > 1073741824)
					{
						std::cerr << "Warning: Very large client_max_body_size: " 
							<< loc.client_max_body_size << " bytes in location " 
							<< loc.path << ". This might cause memory issues." << std::endl;
					}
					if (config.client_max_body_size > 0 && loc.client_max_body_size > config.client_max_body_size)
					{
						std::cerr << "Warning: Location client_max_body_size (" 
							<< loc.client_max_body_size << ") is larger than server client_max_body_size ("
							<< config.client_max_body_size << ") in location " << loc.path 
							<< ". Server limit will take precedence." << std::endl;
					}
				}
			}
		}
	}
}

void validateErrorPage(const std::vector<Server>& servers)
{
	for (const auto& server : servers)
	{
		for (const auto& config : server.conf)
		{
			for (const auto& [errorCode, errorPage] : config.error_page)
			{
				if (errorCode < 100 || errorCode > 599)
				{
					std::cerr << "Warning: Invalid HTTP error code: " 
						<< errorCode << " in server " 
						<< (config.server_name.empty() ? "[empty]" : config.server_name) << std::endl;
				}
				
				try {
					std::string errorPagePath = config.root + "/" + errorPage;
					if (!std::filesystem::exists(errorPagePath))
					{
						std::cerr << "Warning: Error page file does not exist: " 
							<< errorPagePath << " for error code " << errorCode 
							<< " in server " << (config.server_name.empty() ? "[empty]" : config.server_name) << std::endl;
					}
				} catch (const std::exception& e) {
					std::cerr << "Error checking error page file: " << e.what() << std::endl;
				}
			}
			
			for (const auto& loc : config.locations)
			{
				for (const auto& [errorCode, errorPage] : loc.error_page)
				{
					if (errorCode < 100 || errorCode > 599)
					{
						std::cerr << "Warning: Invalid HTTP error code: " 
							<< errorCode << " in location " << loc.path << std::endl;
					}
					
					try {
						std::string basePath = !loc.root.empty() ? loc.root : 
												(!loc.alias.empty() ? loc.alias : config.root);
						std::string errorPagePath = basePath + "/" + errorPage;
						if (!std::filesystem::exists(errorPagePath))
						{
							std::cerr << "Warning: Location error page file does not exist: " 
								<< errorPagePath << " for error code " << errorCode 
								<< " in location " << loc.path << std::endl;
						}
					} catch (const std::exception& e) {
						std::cerr << "Error checking location error page file: " << e.what() << std::endl;
					}
				}
			}
		}
	}
}

void validateAutoindex(const std::vector<Server>& servers)
{
	for (const auto& server : servers)
	{
		for (const auto& config : server.conf)
		{
			if (config.autoindex)
			{
				if (!std::filesystem::exists(config.root) || !std::filesystem::is_directory(config.root))
				{
					std::cerr << "Warning: Autoindex is enabled but root directory does not exist: " 
						<< config.root << " in server " << (config.server_name.empty() ? "[empty]" : config.server_name) << std::endl;
				}
			}
			
			for (const auto& loc : config.locations)
			{
				if (loc.autoindex)
				{
					if (!loc.root.empty())
					{
						if (!std::filesystem::exists(loc.root) || !std::filesystem::is_directory(loc.root))
						{
							std::cerr << "Warning: Autoindex is enabled but location root directory does not exist: " 
								<< loc.root << " in location " << loc.path << std::endl;
						}
					}
					else if (!loc.alias.empty())
					{
						if (!std::filesystem::exists(loc.alias) || !std::filesystem::is_directory(loc.alias))
						{
							std::cerr << "Warning: Autoindex is enabled but location alias directory does not exist: " 
								<< loc.alias << " in location " << loc.path << std::endl;
						}
					}
				}
			}
		}
	}
}

void validateConfigurations(std::vector<Server>& servers) 
{
	if (servers.empty())
		throw std::runtime_error("Error: No valid server configurations found");

	expand_relative_roots(servers);

	try
	{
		validatePort(servers);
		validateRootAlias(servers);
		validateHost(servers);
		validateRoot(servers);
		validateIndex(servers);
		validateUploadDir(servers);
		validateDavMethods(servers);
		validateClientMaxBodySize(servers);
		validateErrorPage(servers);
		validateAutoindex(servers);
		validateLocations(servers);
		
		std::cout << "Configuration validation completed successfully." << std::endl;
	}
	catch (const std::exception& e) 
	{
		std::cerr << e.what() << std::endl;
		std::cerr << "Server will not start due to configuration issues." << std::endl;
		throw;
	}
}