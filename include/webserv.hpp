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
#include <unordered_map>
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
#include <arpa/inet.h>
#include <netdb.h>
// #include <netinet/in.h>

#include <sys/socket.h>
// #include <netinet/in.h>
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

struct ServerConfig;
struct LocationConfig;
struct Connection;
struct Server;

struct Response
{
	const ServerConfig*		server_config = nullptr;
	const LocationConfig*	location_config = nullptr;
	std::string				header;
	std::string				status_text;
	std::string				response_target;
	std::string				location;
	std::string				content_length;
	std::string				content_type = "application/octet-stream";
	std::vector<char>		buffer;
	std::string 			connection;

	bool								directory_listing = false;
	std::string							str_body;
	std::shared_ptr<std::ifstream>		ifs_body;

	void	set_location_config(const std::string& request_target);
	void	set_response_target(std::string request_target, int& status_code);
	void	init_body(int& status_code, const Request& request, const uint16_t port);
	void	init_error_body(int& status_code, const Request& request, const uint16_t port);
	void	generate_directory_listing(const Request& request, const uint16_t port);
	void	generate_error_page(const int status_code);
	void	set_status_text(const int status_code);
	void	set_content_type(void);
};

struct Connection
{
	int					fd;
	const Server*		server = nullptr;
	const ServerConfig*	server_config = nullptr;
	short				events = POLLIN;
	bool				close = false;
	int					status_code = 0;

	Request				request;
	Response			response;

	std::chrono::steady_clock::time_point	timeout;

	short*			revents = nullptr;

	std::vector<char>	buffer;

	Connection(Server* server);
	~Connection(void);

	void	set_server_config(void);
	void	receive(void);
	void	respond(void);
};

struct LocationConfig
{
	std::string										path;
	std::string										root;
	std::set<std::string>							allow_methods;
	bool											autoindex = false;
	std::string										index;
	std::string										client_body_temp_path;
	std::string										fastcgi_param;
	size_t											client_max_body_size = 0;
	std::map<int, std::string>						error_page;
};

struct ServerConfig
{
	in_addr											host;
	std::string										host_str;
	uint16_t										port = 80;
	std::string										root = std::filesystem::current_path().string() + "/html/";
	std::string										index = "index.html";
	std::map<int, std::string>						error_page;
	size_t											client_max_body_size = 0;
	std::string										server_name;
	bool											autoindex = false;
	std::vector<LocationConfig>						locations;
};

struct Server
{
	std::vector<ServerConfig>						conf;
	in_addr											host;
	std::string										host_str;
	uint16_t										port;
	std::list<Connection>							connections;
	int												socket = -1;
	sockaddr_in										sockaddr;
	std::chrono::seconds							request_timeout = std::chrono::seconds(10);
	std::chrono::seconds							response_timeout = std::chrono::seconds(10);

	short*											revents;

	void	accept_connection(void);
	void	clean_connections(void);

	~Server(void){if (socket != -1) close(socket);}
};

/************************************************/
// src/parser/
/************************************************/

//parser.cpp
void	parser(std::vector<Server>& data, std::string confPath);

//utils.cpp
std::string		removeSpaces(const std::string& str);
std::string		removeComments(const std::string& str);
void			printData(const std::vector<Server>& servers);

//validation.cpp
in_addr			host_string_to_in_addr(const std::string& host);
bool			isValidLocationKey(const std::string& key);
bool			isValidServerKey(const std::string& key);
bool			validateConfigurations(std::vector<Server>& servers);

//init_sockets.cpp
void			init_sockaddr(Server& server);
void			init_sockets(std::vector<Server>& servers);

/************************************************/

void	parse_request(Request& request, int& status_code);
bool	parse_start_line(Request& request , std::istringstream& header, int& status_code);