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
#include <csignal>
#include <filesystem>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>


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
};

struct Response
{
	std::string			header;
	std::string			status_text;
	std::string			body_path;
	std::string			content_type = "application/octet-stream";
	std::ifstream*		ifs_body = nullptr;
	std::vector<char>	buffer;
	~Response(void){delete ifs_body;}
	void	set_body_path(int& status_code, const std::string& request_target);
	void	set_content_type(void);
	void	set_status_text(const int status_code);

	std::string 		connection;
};

struct Server;

struct Connection
{
	int				fd;
	const Server*	server;
	short			events = POLLIN;
	bool			close = false;
	int				status_code = 0;

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

struct LocationConfig
{
	std::string					locationPath;
	std::string					redirectURL;
	std::string					documentRoot;
	bool						autoIndexEnabled;
	std::string					defaultFile;
	std::string					uploadDirectory;
	std::string					cgiHandlerExtension;
	std::vector<std::string>	allowedMethods;
};
struct ServerConfig
{
	std::string					serverAddress;
	uint16_t					port;
	std::map<int, std::string>	errorPages;
	size_t						maxClientBodySize;
	std::vector<std::string>	serverNames;
	std::vector<LocationConfig>	locationBlocks;
};

// (Aris)von mir aus kann der location enfernet werden wenn ihr das fur den momment nicht mehr benotigt
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
	int							socket = -1;
	std::map<std::string, ServerConfig> servConf; // (Aris)neuer variable wo die server gespeichert werden. 
	std::multimap<std::string, std::string> config; // (Aris)von mir aus kann weg
	std::vector<Location>		locations; //(Aris)von mir aus kann weg
	std::list<Connection>	 	connections; //(Aris)von mir aus kann weg
	uint16_t					port; //(Aris)von mir aus kann weg
	int							socket;
	sockaddr_in					sockaddr;
	std::chrono::seconds		request_timeout = std::chrono::seconds(10);
	std::chrono::seconds		response_timeout = std::chrono::seconds(10);

	short*						revents;

	void	accept_connection(void);
	void	clean_connections(void);

	~Server(void){if (socket != -1) close(socket);}
};

// src/parser/parser.cpp
void	parser(std::vector<Server>& data, std::string confPath);

void	parse_request(Request& request, int& status_code);
bool	parse_start_line(Request& request , std::istringstream& header, int& status_code);
