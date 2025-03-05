#include "webserv.hpp"

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
	else
		portPart = value;

	config.host = hostPart;
	config.port = static_cast<uint16_t>(std::stoi(portPart));
	sharedHost = hostPart;
	sharedPort = config.port;
}

void saveLocationConfig(LocationConfig& location, const std::string& line, const std::string& path) {
	location.path = path;

	if (line.find(';') == std::string::npos)
		throw std::runtime_error("Missing semicolon in configuration: " + line);

	size_t sepPos = line.find(" ");
	if (sepPos != std::string::npos)
	{
		std::string key = line.substr(0, sepPos);
		std::string value = removeSpaces(line.substr(sepPos + 1));
		if (value.back() == ';')
			value.pop_back();
		
		if (!isValidLocationKey(key))
			throw std::runtime_error("Invalid location configuration key: " + key);
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
		else
			location.client_max_body_size = std::stoull(value);
	}
}

void saveServerConfig(ServerConfig& config, const std::string& line, std::string& sharedHost, uint16_t& sharedPort)
{
	if (line.find(';') == std::string::npos && line.find("location") != 0)
		throw std::runtime_error("Missing semicolon in configuration: " + line);
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
			parseListen(config, value, sharedHost, sharedPort);
		else if (key == "server_name")
		{
			std::istringstream nameStream(value);
			std::string name;
			while (nameStream >> name)
				config.server_name.insert(name);
		}
		else if (key == "root")
			config.root = value;
		else if (key == "index")
			config.index = value;
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
	for (auto& [path, location] : serverConfig.locations)
	{
		if (location.root.empty() && !serverConfig.root.empty())
			location.root = serverConfig.root;
		
		if (location.index.empty() && !serverConfig.index.empty())
			location.index = serverConfig.index;
	}
}

void extractMultiPortServerData(std::vector<Server>& servers, const std::string& block)
{
	std::vector<std::string> configLines;
	std::vector<std::pair<std::string, std::string>> locationLines;
	extractServerBlockLines(block, configLines, locationLines);

	Server newServer;
	ServerConfig serverConfig;
	std::string host = "0.0.0.0";
	uint16_t port = 80;

	processServerConfig(configLines, serverConfig, host, port);
	newServer.host = host;
	newServer.port = port;

	processLocationConfigs(locationLines, serverConfig);

	std::string serverKey;
	if (serverConfig.server_name.empty())
		serverKey = "(NONAME)" + host + ":" + std::to_string(port);
	else
		serverKey = *serverConfig.server_name.begin();
	if (newServer.conf.empty())
		newServer.default_server_name = serverKey;
	newServer.conf[serverKey] = serverConfig;
	servers.push_back(newServer);
}


void groupServersByPort(const std::vector<Server>& allServers, std::vector<Server>& servers)
{
	std::map<std::pair<std::string, uint16_t>, std::vector<const Server*>> hostPortGroups;

	for (const Server& server : allServers)
		hostPortGroups[{server.host, server.port}].push_back(&server);
	
	servers.clear();
	
	for (auto& [hostPort, portServers] : hostPortGroups)
	{
		Server combinedServer;
		combinedServer.host = hostPort.first;
		combinedServer.port = hostPort.second;
		
		std::set<std::string> seenNames;
		bool firstServerHasName = false;
		bool defaultSet = false;
		
		if (!portServers.empty() && !portServers[0]->conf.empty())
		{
			auto firstServerConf = portServers[0]->conf.begin();
			firstServerHasName = !firstServerConf->first.empty();
		}
		for (size_t i = 0; i < portServers.size(); ++i)
		{
			const Server* server = portServers[i];
			for (const auto& [name, config] : server->conf)
			{
				if (i > 0 && name.empty() && firstServerHasName)
				{
					std::cerr << "Warnung: Server ohne Namen auf IP " << combinedServer.host 
							<< " und Port " << combinedServer.port 
							<< " wird ignoriert, da bereits ein Server mit Namen definiert wurde." 
							<< std::endl;
					continue;
				}
				if (seenNames.find(name) != seenNames.end())
				{
					std::cerr << "Warnung: Server mit Namen '" << (name.empty() ? "DEFAULT" : name)
							<< "' auf IP " << combinedServer.host 
							<< " und Port " << combinedServer.port 
							<< " ist bereits definiert. Die erste Definition wird verwendet." 
							<< std::endl;
					continue;
				}
				combinedServer.conf[name] = config;
				seenNames.insert(name);
				if (!defaultSet)
				{
					combinedServer.default_server_name = name;
					defaultSet = true;
				}
			}
		}
		servers.push_back(combinedServer);
	}
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
		std::vector<Server> allServers;
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
				std::cerr << "Error parsing server block: " << e.what() << std::endl;
				std::cerr << "Invalid config file!" << std::endl;
				servers.clear();
				return;
			}
			++it;
		}
		groupServersByPort(allServers, servers);
		if (!validateConfigurations(servers))
		{
			servers.clear();
			return;
		}
		printData(servers);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Parser error: " << e.what() << std::endl;
		servers.clear();
	}
}