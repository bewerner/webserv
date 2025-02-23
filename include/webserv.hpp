#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <map>
#include <regex>
#include <set>
#include <cstdint>
#include <chrono>
#include <algorithm>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

// typedef std::chrono::steady_clock::time_point time_point;
// #define BUFFER_SIZE (size_t)1024*16
#define BUFFER_SIZE (size_t)1024*64
// #define BUFFER_SIZE (size_t)2999999
// #define BUFFER_SIZE (size_t)1

struct Location
{
	std::string path;
	std::multimap<std::string, std::string> location_config;
};

struct Server
{
	std::multimap<std::string, std::string> config;
	std::vector<Location>	locations;
	uint16_t				port;
	int						socket;
	sockaddr_in				sockaddr;
	std::chrono::seconds	request_timeout;
};

struct Connection
{
	int				fd;
	const Server*	server;

	std::string		request_header;
	bool			request_header_received;
	size_t			request_body_content_length;
	std::string		request_body;
	bool			request_received;

	std::string			response_header;
	std::ifstream*		ifs_body;
	std::vector<char>	buffer;

	short			events;

	std::chrono::steady_clock::time_point	timeout;
	bool			close;
};

// src/parser/parser.cpp
void parser(std::vector<Server>& data, std::string confPath);
