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
#include <cctype>

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

struct Request
{
	std::string		request_header; // todo
	std::string		method;
	std::string		request_target;
	std::string		protocol;
	std::string		host;
	std::string 	connection;
	std::string 	content_type;
	size_t			content_length;
	int				status_code;
};

struct Connection
{
	int				fd;
	const Server*	server;
	Request			request;
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

// src/parser/parser.cpp
void	parser(std::vector<Server>& data, std::string confPath);

void	parse_request(Connection& connection, std::string& request_header);