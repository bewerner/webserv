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
#include <memory>


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
// #define BUFFER_SIZE (size_t)1024*64
#define BUFFER_SIZE (size_t)1024*1024
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
	std::shared_ptr<std::ifstream>		ifs_body;
	std::vector<char>	buffer;
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
	std::string								path;
	std::string								root;
	std::set<std::string>					allow_methods;
	bool									autoindex = false;
	std::string								index;
	std::string								client_body_temp_path;
	std::string				 				fastcgi_param;
	size_t									client_max_body_size = 0;
};

struct ServerConfig
{
	std::string								host = "0.0.0.0";
	uint16_t								port = 80;
	std::string								root;
	std::string								index;
	std::map<int, std::string>				error_page;
	size_t									client_max_body_size = 0;
	std::set<std::string>					server_name;
	std::map<std::string, LocationConfig>	locations;
};

struct Server
{
	std::map<std::string, ServerConfig>		conf;
	// nur eine kleine hilfe weil ich muste schauhen... ob das sinn macht die den host hier zu geben 
	// es macht sinn und du kannst den host so zuweisen:
	// in_addr_t addr = inet_addr(server.host.c_str());
	// server.sockaddr.sin_addr.s_addr = addr;
	// ich glaube <arpa/inet.h> muss includiert sein ich weiss nicht ob  <netinet/in.h> reicht.
	std::string								host = "0.0.0.0";
	uint16_t								port;
	std::list<Connection>					connections;
	int										socket = -1;
	sockaddr_in								sockaddr;
	std::chrono::seconds					request_timeout = std::chrono::seconds(10);
	std::chrono::seconds					response_timeout = std::chrono::seconds(10);

	short*									revents;

	void	accept_connection(void);
	void	clean_connections(void);

	~Server(void){if (socket != -1) close(socket);}
};

/************************************************/
// src/parser/
/************************************************/

//parser.cpp
void	parser(std::vector<Server>& data, std::string confPath);
void	parse_request(Request& request, int& status_code);
bool	parse_start_line(Request& request , std::istringstream& header, int& status_code);

//utils.cpp
std::string removeSpaces(const std::string& str);
std::string removeComments(const std::string& str);
void printData(const std::vector<Server>& servers);

//validation.cpp
bool isValidLocationKey(const std::string& key);
bool isValidServerKey(const std::string& key);
void validateConfigurations(const std::vector<Server>& servers);

/************************************************/
