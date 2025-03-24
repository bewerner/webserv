#pragma once

#include <iostream>
#include <ios>
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
#include <ctime>

#include <sys/socket.h>
#include <sys/errno.h>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>


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
	std::string 	transfer_encoding;
	size_t			content_length = 0;
	bool			content_length_specified = false;
};

struct ServerConfig;
struct LocationConfig;
struct Connection;
struct Server;

struct CGI
{
	int		pipe_into_cgi[2] = {-1, -1};
	int		pipe_from_cgi[2] = {-1, -1};
	pid_t	pid = -1;
	bool	fail = false;

	short*	revents_write_into_cgi = nullptr;
	short*	revents_read_from_cgi = nullptr;

	void	init_pipes(void);
	void	fork(void);
	void	setup_io(void);
	~CGI(void);
};

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
	std::string				transfer_encoding;
	std::string 			connection;
	bool					header_sent = false;

	CGI									cgi;
	bool								cgi_header_extracted = false;
	bool								cgi_EOF = false;
	std::string							cgi_header;
	std::string							str_body;
	std::shared_ptr<std::ifstream>		ifs_body;

	std::string							path_info;

	void	set_location_config(const std::string& request_target);
	void	set_response_target(std::string request_target, int& status_code);
	void	init_body(int& status_code, const Request& request, const uint16_t port, char** envp);
	void	init_error_body(int& status_code, const Request& request, const uint16_t port, char** envp);
	void	create_header(const int status_code);
	void	generate_directory_listing(const Request& request);
	void	generate_error_page(const int status_code);
	void	set_status_text(const int status_code);
	void	set_content_type(void);
	void	extract_path_info(std::string& request_target);
	void	init_cgi(int& status_code, char** envp);
	void	extract_cgi_header(std::array<char, BUFFER_SIZE>& buf, ssize_t& size, int& status_code);
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

	short*				revents = nullptr;

	std::vector<char>	buffer;

	Connection(Server* server);
	~Connection(void);

	void	set_server_config(void);
	void	receive(void);
	void	respond(void);

	void	receive_header(void);
	void	receive_body(void);
	void	init_response(void);
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
	std::chrono::seconds							request_timeout = std::chrono::seconds(35);
	std::chrono::seconds							response_timeout = std::chrono::seconds(35);
	char**									envp;

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
// src/
/************************************************/

//utils.cpp
void	normalize_path(std::string& path);
bool	collapse_absolute_path(std::string& path);

//Request.cpp
void	parse_request(Request& request, int& status_code);
bool	parse_start_line(Request& request , std::istringstream& header, int& status_code);
