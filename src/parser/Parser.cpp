#include "webserv.hpp"

void printData(const std::vector<Server>& servers)
{
	std::cout << "Number of servers: " << servers.size() << '\n';
	size_t serverNum = 1;

	for (const auto& server : servers)
	{
		std::cout << std::endl;
		std::cout << "Server " << serverNum++ << " (Port " << server.port << "):\n";
		
		for (const auto& [name, config] : server.conf)
		{
			std::cout << "Address: " << config.addr << "\n";
			std::cout << "Port: " << config.listen << "\n";
			
			std::cout << "Max Client Body Size: " << config.client_max_body_size << "\n";
			
			std::cout << "Server Name: ";
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
				
				auto redirectIt = loc.directives.find("redirect");
				std::string redirectUrl = (redirectIt != loc.directives.end()) ? redirectIt->second : "";
				std::cout << "  Redirect URL: " << redirectUrl << "\n";
				
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

bool isValidLocationKey(const std::string& key) {
	static const std::set<std::string> validKeys =
	{
		"redirect",
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

bool isValidServerKey(const std::string& key) {
	static const std::set<std::string> validKeys =
	{
		"listen",
		"server_name",
		"host",
		"server_address",
		"client_max_body_size",
		"error_page"
	};
	return validKeys.find(key) != validKeys.end();
}

void saveLocationConfig(LocationConfig& location, const std::string& line, const std::string& path) {
	location.path = path;

	size_t sepPos = line.find(" ");
	if (sepPos != std::string::npos)
	{
		std::string key = line.substr(0, sepPos);
		std::string value = removeSpaces(line.substr(sepPos + 1));
		if (value.back() == ';')
			value.pop_back();
		
		if (!isValidLocationKey(key))
			throw std::runtime_error("Invalid location configuration key: " + key);
		
		if (key == "redirect")
			location.directives.insert({"redirect", value});
		else if (key == "root")
			location.root = value;
		else if (key == "autoindex")
			location.autoindex = (value == "on");
		else if (key == "index")
			location.index = value;
		else if (key == "upload_dir")
			location.client_body_temp_path = value;
		else if (key == "cgi_extension")
			location.fastcgi_param = value;
		else if (key == "allow_methods")
		{
			std::istringstream methodStream(value);
			std::string method;
			while (methodStream >> method)
				location.allow_methods.insert(method);
		}
		else if (key == "client_max_body_size")
			location.client_body_temp_path = value;
		else 
		{
			location.directives.insert({key, value});
		}
	}
}

void saveServerConfig(ServerConfig& config, const std::string& line, uint16_t port = 0) {
	size_t sepPos = line.find(" ");
	if (sepPos != std::string::npos)
	{
		std::string key = line.substr(0, sepPos);
		std::string value = removeSpaces(line.substr(sepPos + 1));
		if (value.back() == ';')
			value.pop_back();
		
		if (!isValidServerKey(key))
			throw std::runtime_error("Invalid server configuration key: " + key);
		
		if (key == "listen")
		{
			if (port == 0)
			{
				std::istringstream portStream(value);
				std::string portStr;
				if (portStream >> portStr)
					config.listen = static_cast<uint16_t>(std::stoi(portStr));
			}
			else
			{
				config.listen = port;
			}
		}
		else if (key == "server_name")
		{
			std::istringstream nameStream(value);
			std::string name;
			while (nameStream >> name)
				config.server_name.push_back(name);
		}
		else if (key == "host" || key == "server_address")
			config.addr = value;
		else if (key == "client_max_body_size")
			config.client_max_body_size = std::stoull(value);
		else if (key == "error_page")
		{
			std::istringstream errorStream(value);
			int errorCode;
			std::string errorPage;
			if (errorStream >> errorCode >> errorPage)
				config.error_page[errorCode] = errorPage;
		}
	}
}

void extractMultiPortServerData(std::vector<Server>& servers, const std::string& block)
{
	std::string cleanBlock = block;
	size_t serverPos = cleanBlock.find("server");
	size_t openBracePos = cleanBlock.find("{", serverPos);
	if (serverPos != std::string::npos && openBracePos != std::string::npos)
		cleanBlock = cleanBlock.substr(openBracePos + 1);

	std::istringstream stream(cleanBlock);
	std::string line;
	bool insideLocation = false;
	std::string currentPath;

	std::vector<uint16_t> ports;
	std::vector<std::string> configLines;
	std::vector<std::pair<std::string, std::string>> locationLines;

	while (std::getline(stream, line))
	{
		line = removeSpaces(line);
		if (line.empty())
			continue;
		if (line.find("server") == 0 && line.find("server_name") != 0)
			continue;
		if (line.find("location") == 0)
		{
			size_t pathStart = line.find_first_of(" ") + 1;
			size_t pathEnd = line.find_first_of("{") - 1;
			currentPath = removeSpaces(line.substr(pathStart, pathEnd - pathStart));
			insideLocation = true;
			continue;
		}
		if (line == "}")
		{
			insideLocation = false;
			continue;
		}
		if (!insideLocation && line.find("listen") == 0)
		{
			size_t sepPos = line.find(" ");
			if (sepPos != std::string::npos)
			{
				std::string value = removeSpaces(line.substr(sepPos + 1));
				if (!value.empty() && value.back() == ';')
					value.pop_back();
				std::istringstream portStream(value);
				std::string portStr;
				while (portStream >> portStr)
				{
					ports.push_back(static_cast<uint16_t>(std::stoi(portStr)));
					std::cout << "Added port: " << portStr << std::endl;
				}
			}
			continue;
		}
		if (insideLocation)
			locationLines.push_back({currentPath, line});
		else
			configLines.push_back(line);
	}

	if (ports.empty())
		ports.push_back(80);
	for (uint16_t port : ports)
	{
		Server newServer;
		ServerConfig serverConfig;
		newServer.port = port;
		serverConfig.listen = port;
		for (const auto& configLine : configLines)
		{
			try
			{
				if (!configLine.empty())
				{
					std::cout << "Parsing config line: " << configLine << std::endl;
					saveServerConfig(serverConfig, configLine, port);
				}
			}
			catch (const std::exception& e)
			{
				throw std::runtime_error("Error in server config: " + std::string(e.what()));
			}
		}
		std::map<std::string, LocationConfig> locations;
		for (const auto& [path, locLine] : locationLines)
		{
			if (locations.find(path) == locations.end())
				locations[path] = LocationConfig();
			try
			{
				if (!locLine.empty())
					saveLocationConfig(locations[path], locLine, path);
			}
			catch (const std::exception& e)
			{
				throw std::runtime_error("Error in location config: " + std::string(e.what()));
			}
		}
		serverConfig.locations = locations;
		if (serverConfig.server_name.empty())
		{
			std::cout << "No server_name specified for port " << port << std::endl;
			serverConfig.server_name.push_back("localhost");
			std::cout << "Using default server_name: localhost" << std::endl;
		}
		else
		{
			std::cout << "Final server names:" << std::endl;
			for (const auto& name : serverConfig.server_name)
				std::cout << "  - " << name << std::endl;
		}
		for (const auto& name : serverConfig.server_name)
		{
			newServer.conf[name] = serverConfig;
			std::cout << "Added server config with name: " << name << std::endl;
		}
		servers.push_back(newServer);
		std::cout << "Added server with port: " << port << std::endl;
	}
}


void validateConfigurations(const std::vector<Server>& servers) {
	(void)servers;
	// Locations Validate TODO
	// ValidatePath(servers);
	// ValidateRoot(servers);
	// ValidateDirectives(servers);
	// ValidateAllowMethos(servers);
	// ValidateAutoIndex(servers);
	// ValidateIndex(servers);
	// ValidateClientBodyTempPath(servers);
	// ValidateFastcgiParam(servers);

	// Server Validate TODO
	// ValidateAddr(servers);
	// ValidateListe(servers);
	// ValidateErrorPage(servers);
	// ValidateClientMaxBodySize(servers);
	// ValidateServerName(servers);
	// ValidateLocations(servers);
}

void parser(std::vector<Server>& servers, std::string confPath)
{
	try 
	{
		std::ifstream file(confPath);
		if (!file.is_open())
			throw std::runtime_error("Failed to open config file: " + confPath);
		std::string content((std::istreambuf_iterator<char>(file)),
							std::istreambuf_iterator<char>());

		content = removeComments(content);
		content = removeSpaces(content);

		std::regex serverPattern(R"(server\s*?\{[\s\S]*?\}\s*?(?=server\s*?\{|$))");
		std::sregex_iterator it(content.begin(), content.end(), serverPattern);
		std::sregex_iterator end;

		std::cout << "Content after cleaning: " << content << std::endl;
		std::cout << "Starting to parse server blocks..." << std::endl;

		while (it != end)
		{
			std::string serverBlock = it->str();
			std::cout << "Found server block: " << serverBlock << std::endl;
			try
			{
				extractMultiPortServerData(servers, serverBlock);
			}
			catch (const std::exception& e)
			{
				std::cerr << "Error parsing server block: " << e.what() << std::endl;
				std::cerr << "Invalid config file!" << std::endl;
				servers.clear();
				return;
			}
			++it;
		}
		validateConfigurations(servers);
		printData(servers);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Parser error: " << e.what() << std::endl;
		servers.clear();
	}
}