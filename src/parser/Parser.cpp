#include "webserv.hpp"

bool isNumeric(const std::string& str)
{
	if (str.empty())
		return false;

	for (size_t i = 0; i < str.size(); ++i)
	{
		if (i == 0 && str[i] == '-' && str.size() > 1)
			continue;
		if (!std::isdigit(str[i]))
			return false;
	}
	return true;
}

bool safeStringToUInt16(const std::string& str, uint16_t& result)
{
	if (!isNumeric(str))
		return false;

	try 
	{
		long value = std::stol(str);
		if (value < 0 || value > UINT16_MAX)
			return false;
		
		result = static_cast<uint16_t>(value);
		return true;
	}
	catch (const std::exception&) 
	{
		return false;
	}
}

bool safeStringToSizeT(const std::string& str, size_t& result)
{
	if (!isNumeric(str))
		return false;

	try
	{
		long long value = std::stoll(str);
		if (value < 0)
			return false;
		
		result = static_cast<size_t>(value);
		return true;
	}
	catch (const std::exception&)
	{
		return false;
	}
}

void parseListen(ServerConfig& config, const std::string& value, std::string& sharedHost, uint16_t& sharedPort)
{
	std::string hostPart = "0.0.0.0";
	std::string portPart = "80";
	size_t colonPos = value.find(':');
	if (colonPos != std::string::npos)
	{
		hostPart = value.substr(0, colonPos);
		portPart = value.substr(colonPos + 1);
	}
	else if (isNumeric(value))
		portPart = value;
	else
		hostPart = value;
	uint16_t port;
	if (!safeStringToUInt16(portPart, port)) {
		throw std::runtime_error("Invalid port number: " + portPart);
	}
	config.host_str = hostPart;
	try
	{
		config.host = host_string_to_in_addr(hostPart);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Warning: Host '" << hostPart << "' could not be resolved. Using 0.0.0.0" << std::endl;
		config.host.s_addr = INADDR_ANY;
	}
	config.port = port;
	sharedHost = hostPart;
	sharedPort = config.port;
}

void saveLocationConfig(LocationConfig& location, const std::string& line, const std::string& path)
{
	location.path = path;

	if (line.find(';') == std::string::npos && line.find("location") != 0)
		throw std::runtime_error("Missing semicolon in configuration: " + line);

	size_t sepPos = line.find(" ");

	if (sepPos == std::string::npos)
		throw std::runtime_error("Invalid format (no space to separate key and value): " + line);

	std::string key = line.substr(0, sepPos);

	if (key.find(';') != std::string::npos)
		throw std::runtime_error("Invalid semicolon placement in key: " + key);

	std::string value = removeSpaces(line.substr(sepPos + 1));

	if (value.empty() || value == ";")
		throw std::runtime_error("Missing value for key: " + key);

	bool endsWithSemicolon = false;
	if (!value.empty() && value.back() == ';')
	{
		endsWithSemicolon = true;
		value.pop_back();
	}

	if (value.empty())
		throw std::runtime_error("Missing value for key: " + key);

	if (value.find(';') != std::string::npos)
		throw std::runtime_error("Invalid semicolon placement in value: " + line);

	if (!endsWithSemicolon && line.find("location") != 0)
		throw std::runtime_error("Missing semicolon at the end of line: " + line);

	if (!isValidLocationKey(key))
		throw std::runtime_error("Invalid location configuration key: " + key);

	if (key == "root")
		location.root = value;
	else if (key == "alias")
		location.alias = value;
	else if (key == "autoindex")
	{
		if (value != "on" && value != "off")
			throw std::runtime_error("Invalid autoindex value. Expected 'on' or 'off', got: " + value);
		location.autoindex = (value == "on");
	}
	else if (key == "index")
		location.index = value;
	else if (key == "upload_dir")
		location.client_body_temp_path = value;
	else if (key == "cgi")
	{
		if (value != "on" && value != "off")
			throw std::runtime_error("Invalid cgi value. Expected 'on' or 'off', got: " + value);
		location.cgi = (value == "on");
	}
	else if (key == "dav_methods")
	{
		std::istringstream methodStream(value);
		std::string method;
		while (methodStream >> method)
		{
			if (method != "GET" && method != "POST" && method != "DELETE")
			{
				throw std::runtime_error("Invalid HTTP method in dav_methods: " + method);
			}
			location.dav_methods.insert(method);
		}
		
		if (location.dav_methods.empty())
			throw std::runtime_error("No valid methods specified in dav_methods directive");
	}
	else if (key == "client_max_body_size")
	{
		size_t size;
		if (!safeStringToSizeT(value, size))
			throw std::runtime_error("Invalid client_max_body_size value: " + value);
		location.client_max_body_size = size;
	}
}

void saveServerConfig(ServerConfig& config, const std::string& line, std::string& sharedHost, uint16_t& sharedPort)
{
	if (line.find(';') == std::string::npos && line.find("location") != 0)
		throw std::runtime_error("Missing semicolon in configuration: " + line);

	size_t sepPos = line.find(" ");

	if (sepPos == std::string::npos)
		throw std::runtime_error("Invalid format (no space to separate key and value): " + line);

	std::string key = line.substr(0, sepPos);

	if (key.find(';') != std::string::npos)
		throw std::runtime_error("Invalid semicolon placement in key: " + key);

	std::string value = removeSpaces(line.substr(sepPos + 1));

	if (value.empty() || value == ";")
		throw std::runtime_error("Missing value for key: " + key);

	bool endsWithSemicolon = false;
	if (!value.empty() && value.back() == ';')
	{
		endsWithSemicolon = true;
		value.pop_back();
	}

	if (value.empty())
		throw std::runtime_error("Missing value for key: " + key);

	if (value.find(';') != std::string::npos)
		throw std::runtime_error("Invalid semicolon placement in value: " + line);

	if (!endsWithSemicolon && line.find("location") != 0)
		throw std::runtime_error("Missing semicolon at the end of line: " + line);

	if (!isValidServerKey(key))
		throw std::runtime_error("Invalid server configuration key: " + key);

	if (key == "listen")
		parseListen(config, value, sharedHost, sharedPort);
	else if (key == "server_name")
	{
		std::istringstream nameStream(value);
		nameStream >> config.server_name;
	}
	else if (key == "root")
		config.root = value;
	else if (key == "index")
		config.index = value;
	else if (key == "autoindex")
	{
		if (value != "on" && value != "off")
			throw std::runtime_error("Invalid autoindex value. Expected 'on' or 'off', got: " + value);
		config.autoindex = (value == "on");
	}
	else if (key == "client_max_body_size")
	{
		size_t size;
		if (!safeStringToSizeT(value, size))
			throw std::runtime_error("Invalid client_max_body_size value: " + value);
		config.client_max_body_size = size;
	}
	else if (key == "error_page")
	{
		std::istringstream errorStream(value);
		int errorCode;
		std::string errorCodeStr;
		std::string errorPage;
		
		if (!(errorStream >> errorCodeStr) || !isNumeric(errorCodeStr))
			throw std::runtime_error("Invalid error_page format. Expected 'error_page CODE PATH', got: " + value);
		
		try {
			errorCode = std::stoi(errorCodeStr);
			if (errorCode < 100 || errorCode > 599)
				throw std::runtime_error("Invalid HTTP error code in error_page: " + errorCodeStr);
		} catch (const std::exception&) {
			throw std::runtime_error("Invalid HTTP error code in error_page: " + errorCodeStr);
		}

		if (!(errorStream >> errorPage))
			throw std::runtime_error("Missing error page path in error_page directive");
			
		config.error_page[errorCode] = errorPage;
	}
}

void extractServerBlockLines(const std::string& block, std::vector<std::string>& configLines, std::vector<std::pair<std::string, std::string>>& locationLines)
{
	std::string cleanBlock = block;
	size_t serverPos = cleanBlock.find("server");
	size_t openBracePos = cleanBlock.find("{", serverPos);
	if (serverPos != std::string::npos && openBracePos != std::string::npos)
		cleanBlock = cleanBlock.substr(openBracePos + 1);

	std::istringstream stream(cleanBlock);
	std::string line;
	bool insideLocation = false;
	bool expectLocationBrace = false;
	std::string currentPath;

	while (std::getline(stream, line))
	{
		line = removeSpaces(line);
		if (line.empty())
			continue;
		if (line.find("server") == 0 && line.find("server_name") != 0)
			continue;
		if (expectLocationBrace && line == "{")
		{
			expectLocationBrace = false;
			insideLocation = true;
			continue;
		}
		if (line.find("location") == 0)
		{
			size_t pathStart = line.find_first_of(" ") + 1;
			size_t bracePos = line.find("{");
			if (bracePos != std::string::npos)
			{
				size_t pathEnd = bracePos - 1;
				currentPath = removeSpaces(line.substr(pathStart, pathEnd - pathStart));
				insideLocation = true;
			}
			else
			{
				currentPath = removeSpaces(line.substr(pathStart));
				expectLocationBrace = true;
			}
			continue;
		}
		if (line == "}")
		{
			insideLocation = false;
			expectLocationBrace = false;
			continue;
		}
		if (insideLocation)
			locationLines.push_back({currentPath, line});
		else
			configLines.push_back(line);
	}
}

void processServerConfig(const std::vector<std::string>& configLines, ServerConfig& serverConfig, std::string& host, uint16_t& port)
{
	for (const auto& configLine : configLines)
	{
		try
		{
			if (!configLine.empty())
				saveServerConfig(serverConfig, configLine, host, port);
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("Error in server config: " + std::string(e.what()));
		}
	}
}

void processLocationConfigs(const std::vector<std::pair<std::string, std::string>>& locationLines, ServerConfig& serverConfig)
{
	std::map<std::string, LocationConfig> locationsMap;
	for (const auto& [path, locLine] : locationLines)
	{
		if (locationsMap.find(path) == locationsMap.end())
			locationsMap[path] = LocationConfig();
		try
		{
			if (!locLine.empty())
				saveLocationConfig(locationsMap[path], locLine, path);
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("Error in location config: " + std::string(e.what()));
		}
	}
	serverConfig.locations.clear();
	for (auto& [path, location] : locationsMap)
	{
		if (location.root.empty() && !serverConfig.root.empty())
			location.root = serverConfig.root;
		if (location.index.empty() && !serverConfig.index.empty())
			location.index = serverConfig.index;
		if (location.client_max_body_size == 0 && serverConfig.client_max_body_size != 0)
			location.client_max_body_size = serverConfig.client_max_body_size;
		if (!location.autoindex && serverConfig.autoindex)
			location.autoindex = true;
		location.error_page = serverConfig.error_page;
		serverConfig.locations.push_back(location);
	}
}

void extractMultiPortServerData(std::vector<Server>& servers, const std::string& block)
{
	std::vector<std::string> configLines;
	std::vector<std::pair<std::string, std::string>> locationLines;

	try {
		extractServerBlockLines(block, configLines, locationLines);
	}
	catch (const std::exception& e) {
		throw std::runtime_error("Error extracting server block: " + std::string(e.what()));
	}

	ServerConfig baseConfig;
	std::vector<std::pair<std::string, uint16_t>> listenDirectives;
	std::vector<std::string> serverNames;
	bool hasServerNames = false;

	for (const auto& configLine : configLines)
	{
		if (configLine.find("listen") == 0)
		{
			try {
				size_t sepPos = configLine.find(" ");
				if (sepPos != std::string::npos)
				{
					std::string value = removeSpaces(configLine.substr(sepPos + 1));
					if (value.back() == ';')
						value.pop_back();
					
					std::string host = "0.0.0.0";
					std::string portPart = "80";
					size_t colonPos = value.find(':');
					
					if (colonPos != std::string::npos)
					{
						host = value.substr(0, colonPos);
						portPart = value.substr(colonPos + 1);
					}
					else if (isNumeric(value))
						portPart = value;
					else
						host = value;
					
					uint16_t port;
					if (!safeStringToUInt16(portPart, port))
						throw std::runtime_error("Invalid port number: " + portPart);
					
					listenDirectives.push_back({host, port});
				}
			}
			catch (const std::exception& e) {
				throw std::runtime_error("Error in listen directive: " + std::string(e.what()));
			}
		}
	}

	if (listenDirectives.empty())
		listenDirectives.push_back({"0.0.0.0", 80});

	for (const auto& configLine : configLines)
	{
		if (configLine.find("server_name") == 0)
		{
			try {
				size_t sepPos = configLine.find(" ");
				if (sepPos != std::string::npos)
				{
					std::string value = removeSpaces(configLine.substr(sepPos + 1));
					if (value.back() == ';')
						value.pop_back();
					
					std::istringstream nameStream(value);
					std::string name;
					while (nameStream >> name)
					{
						serverNames.push_back(name);
						hasServerNames = true;
					}
				}
			}
			catch (const std::exception& e) {
				throw std::runtime_error("Error in server_name directive: " + std::string(e.what()));
			}
		}
	}

	if (!hasServerNames)
		serverNames.push_back("");

	for (const auto& configLine : configLines)
	{
		if (configLine.find("listen") != 0 && configLine.find("server_name") != 0)
		{
			try
			{
				std::string tempHost = "0.0.0.0";
				uint16_t tempPort = 80;
				
				if (!configLine.empty())
					saveServerConfig(baseConfig, configLine, tempHost, tempPort);
			}
			catch (const std::exception& e)
			{
				throw std::runtime_error("Error in server config: " + std::string(e.what()));
			}
		}
	}

	try 
	{
		processLocationConfigs(locationLines, baseConfig);
	}
	catch (const std::exception& e) 
	{
		throw std::runtime_error("Error processing location configs: " + std::string(e.what()));
	}

	for (const auto& [host_str, port] : listenDirectives)
	{
		for (const std::string& name : serverNames)
		{
			Server newServer;
			try
			{
				newServer.host = host_string_to_in_addr(host_str);
			}
			catch (const std::exception& e)
			{
				std::cerr << "Warning: Host '" << host_str << "' could not be resolved. Using 0.0.0.0" << std::endl;
				newServer.host.s_addr = INADDR_ANY;
			}
			newServer.host_str = host_str;
			newServer.port = port;

			ServerConfig serverConfig = baseConfig;
			try
			{
				serverConfig.host = host_string_to_in_addr(host_str);
			}
			catch (const std::exception& e)
			{
				std::cerr << "Warning: Host '" << host_str << "' could not be resolved. Using 0.0.0.0" << std::endl;
				serverConfig.host.s_addr = INADDR_ANY;
			}
			serverConfig.host_str = host_str;
			serverConfig.port = port;
			serverConfig.server_name = name;
			newServer.conf.push_back(serverConfig);
			servers.push_back(newServer);
		}
	}
}

void groupServersByPort(const std::vector<Server>& allServers, std::vector<Server>& servers)
{
	std::map<std::pair<in_addr_t, uint16_t>, std::vector<const Server*>> hostPortGroups;
	std::map<std::pair<in_addr_t, uint16_t>, const Server*> firstServerPerHostPort;

	for (const Server& server : allServers)
	{
		std::pair<in_addr_t, uint16_t> hostPort = {server.host.s_addr, server.port};
		hostPortGroups[hostPort].push_back(&server);
		
		if (firstServerPerHostPort.find(hostPort) == firstServerPerHostPort.end())
			firstServerPerHostPort[hostPort] = &server;
	}
	servers.clear();
	for (auto& [hostPort, portServers] : hostPortGroups)
	{
		Server combinedServer;
		const Server* firstServer = firstServerPerHostPort[hostPort];
		combinedServer.host = firstServer->host;
		combinedServer.host_str = firstServer->host_str;
		combinedServer.port = hostPort.second;
		std::vector<ServerConfig> allConfigs;
		for (const auto& config : firstServer->conf)
			allConfigs.push_back(config);
		for (const Server* server : portServers)
		{
			if (server == firstServer) continue;
			for (const auto& config : server->conf)
			{
				bool configExists = false;
				for (const auto& existingConfig : allConfigs)
				{
					if (existingConfig.server_name == config.server_name)
					{
						configExists = true;
						break;
					}
				}
				if (!configExists)
					allConfigs.push_back(config);
			}
		}
		combinedServer.conf = allConfigs;
		servers.push_back(combinedServer);
	}
}

void parser(std::vector<Server>& servers, std::string confPath)
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
	std::vector<Server> allServers;

	if (it == end)
		throw std::runtime_error("No server blocks found in configuration file");
		
	while (it != end)
	{
		std::string serverBlock = it->str();
		try
		{
			std::vector<Server> tempServers;
			extractMultiPortServerData(tempServers, serverBlock);
			allServers.insert(allServers.end(), tempServers.begin(), tempServers.end());
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("Error parsing server block: " + std::string(e.what()));
		}
		++it;
	}

	if (allServers.empty())
		throw std::runtime_error("No valid server configurations found");
		
	groupServersByPort(allServers, servers);
	validateConfigurations(servers);
	try
	{
		init_sockets(servers);
	}
	catch (const std::exception& e)
	{
		throw std::runtime_error("Socket initialization error: " + std::string(e.what()));
	}

	// Print configuration data
	printData(servers);
}