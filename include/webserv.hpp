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
#include <cctype>
#include <algorithm>
#include <list>

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

struct Request
{
	std::string		header;
	bool			header_received = false;
	size_t			remaining_bytes = 0;
	std::string		body;
	bool			received = false;
	bool			startline_parsed = false;

	std::string		method;
	std::string		request_target;
	std::string		protocol;
	std::string		host;
	std::string 	connection = "keep-alive";
	std::string 	content_type;
	size_t			content_length = 0;
	int				status_code = 0;
};

struct Response
{
	std::string			header;
	std::ifstream*		ifs_body = nullptr;
	std::vector<char>	buffer;
	~Response(void){delete ifs_body;}

	std::string 		connection;
};

struct Server;

struct Connection
{
	int				fd;
	const Server*	server;
	short			events = POLLIN;
	bool			close = false;

	Request			request;
	Response		response;

	std::chrono::steady_clock::time_point	timeout;

	short*			revents = nullptr;

	std::vector<char>	buffer;

	Connection(Server* server);
	~Connection(void);

	void	receive(void);
	void	respond(void);
};

struct Location
{
	std::string path;
	std::multimap<std::string, std::string> location_config;
};

struct Server
{
	std::multimap<std::string, std::string> config;
	std::vector<Location>		locations;
	std::list<Connection>	 	connections;
	uint16_t					port;
	int							socket;
	sockaddr_in					sockaddr;
	std::chrono::seconds		request_timeout;

	short*						revents;

	void	accept_connection(void);
	void	clean_connections(void);
};

// src/parser/parser.cpp
void	parser(std::vector<Server>& data, std::string confPath);

void	parse_request(Request& request);
bool	parse_start_line(Request& request , std::istringstream& header);
