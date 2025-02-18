#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <regex>
#include <set>
#include <cstdint>
#include <chrono>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>

// typedef std::chrono::steady_clock::time_point time_point;
#define BUFFER_SIZE 1024

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
	pollfd*					poll_fd;
	std::chrono::seconds	request_timeout;
};

struct Connection
{
	int				fd;
	const Server*	server;
	std::string		request_header;
	bool			request_header_complete;
	std::string		request_body;
	bool			request_complete;
	bool			response_complete;
	size_t			request_body_content_length;
	pollfd*			poll_fd;
	bool			close;
	std::chrono::steady_clock::time_point	last_change;
};

void parser(std::vector<Server>& data, std::string confPath);
