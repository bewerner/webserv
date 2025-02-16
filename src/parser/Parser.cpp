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

void parseLocationBlock(Location& location, const std::string& line)
{
	size_t sepPos = line.find(" ");
	if (sepPos != std::string::npos)
	{
		std::string key = line.substr(0, sepPos);
		std::string value = removeSpaces(line.substr(sepPos + 1));
		if (value.back() == ';')
			value.pop_back();
		location.location_config.insert(std::make_pair(key, value));
	}
}

void parseServerConfig(Server& server, const std::string& line)
{
	size_t sepPos = line.find(" ");
	if (sepPos != std::string::npos)
	{
		std::string key = line.substr(0, sepPos);
		std::string value = removeSpaces(line.substr(sepPos + 1));
		if (value.back() == ';')
			value.pop_back();
		server.config.insert(std::make_pair(key, value));
	}
}

void parseServerBlock(Server& server, const std::string& block)
{
	std::istringstream stream(block);
	std::string line;
	bool insideLocation = false;
	Location currentLoc;

	while (std::getline(stream, line))
	{
		line = removeSpaces(line);
		if (line.empty()) continue;
		if (line.find("location") == 0)
		{
			if (insideLocation)
			{
				server.locations.push_back(currentLoc);
				currentLoc = Location();
			}
			size_t pathStart = line.find_first_of(" ") + 1;
			size_t pathEnd = line.find_first_of("{") - 1;
			currentLoc.path = removeSpaces(line.substr(pathStart, pathEnd - pathStart));
			insideLocation = true;
			continue;
		}
		if (line == "}")
		{
			if (insideLocation)
			{
				server.locations.push_back(currentLoc);
				currentLoc = Location();
				insideLocation = false;
			}
			continue;
		}
		if (insideLocation)
			parseLocationBlock(currentLoc, line);
		else
			parseServerConfig(server, line);
	}
	if (insideLocation)
		server.locations.push_back(currentLoc);
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
	std::regex serverPattern(R"(server(\s|\n)*?\{(([^\{\}]|\n)*?\{([^\{\}]|\n)*?\})*?([^\{\}]|\n)*?\})");

	std::sregex_iterator it(content.begin(), content.end(), serverPattern);
	std::sregex_iterator end;

	while (it != end)
	{
		std::string serverBlock = it->str();
		Server newServer;
		parseServerBlock(newServer, serverBlock);
		servers.push_back(newServer);
		++it;
	}
}