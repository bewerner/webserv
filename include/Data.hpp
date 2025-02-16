#pragma once

#include <vector>
#include <map>
#include <string>

struct Location
{
	std::string path;
	std::multimap<std::string, std::string> location_config;
};

struct Server
{
	std::multimap<std::string, std::string> config;
	std::vector<Location> locations;
};

// class Data
// {
// 	public:
// 		std::vector<Server> servers;
// 		Data();
// 		~Data();
// };
