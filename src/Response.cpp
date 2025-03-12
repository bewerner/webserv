#include "webserv.hpp"

void	Response::set_location_config(const std::string& request_target)
{
	size_t	match_length = 0;
	for (const LocationConfig& config : server_config->locations)
	{
		if (request_target.find(config.path) == 0)
		{
			if (config.path.size() > match_length)
			{
				match_length = config.path.size();
				location_config = &(config);
			}
		}
	}
	// std::cout << "location config: \"" << location_config->path << "\"" << std::endl;
	// if (match_length == 0)
	// 	location_config = nullptr;
	if (match_length == 0)
		throw std::logic_error("this should never happen. investigate (in set_location_config function)");
}

// static LocationConfig	get_location_config(const ServerConfig* server_config, const LocationConfig* location_config)
// {
// 	if (location_config)
// 		return (*location_config);
// 	LocationConfig config;
// 	config.path = "/";
// 	config.root = server_config->root;
// 	config.allow_methods = std::set<std::string>({"GET", "POST", "DELETE"});
// 	config.autoindex = server_config->autoindex;
// 	config.index = server_config->index;
// 	config.client_max_body_size = server_config->client_max_body_size;
// 	config.error_page = server_config->error_page;

// 	return (config);
// }

// if directory request
	// absolute index means request target is irrelevant -> search for root+index
	// (directory index && absolute index) means request target and root are irrelevant -> jump to location matching index with index as request target


void	Response::set_response_target(std::string request_target, int& status_code)
{
	// LocationConfig config = get_location_config(server_config, location_config);
	const LocationConfig* config = location_config;

	bool directory_request = (request_target.back() == '/');
	bool absolute_index    = (config->index.front() == '/');
	bool directory_index   = (config->index.back() == '/');


	for (size_t i = 0; directory_request && absolute_index && directory_index; i++) // request target and root are irrelevant -> jump to location matching index with index as request target
	{
		if (i > 9)
		{
			status_code = 500;
			return ;
		}
		set_location_config(config->index);
		// config = get_location_config(server_config, location_config);
		config = location_config;
		request_target = config->index;
		directory_request = (request_target.back() == '/');
		absolute_index    = (config->index.front() == '/');
		directory_index   = (config->index.back() == '/');
		if (directory_request)
			request_target.clear();
	}

	if (directory_request)
	{
		if (absolute_index) // request target is irrelevant -> search for root+index
		{
			response_target = config->root + config->index;
			if (!std::filesystem::exists(response_target))
				response_target = config->root;
		}
		else // index is relative file_index (for example index.html)
		{
			response_target = config->root + request_target + config->index;
			if (!std::filesystem::exists(response_target))
				response_target = config->root + request_target;
			// else if (std::filesystem::is_directory(response_target))
			// {

			// }
		}
	}
	else
		response_target = config->root + request_target;
}

void	Response::init_body(int& status_code, const Request& request, const uint16_t port)
{
	bool directory_request = (response_target.back() == '/');

	if (directory_request)
	{
		if (!std::filesystem::exists(response_target) || !std::filesystem::is_directory(response_target))
		{
			std::cout << "-------------------------------------++++ " << response_target << std::endl;
			status_code = 404;
		}
		else if (location_config->autoindex)
			generate_directory_listing(request, port);
		else
			status_code = 403;
		return ;
	}

	if (std::filesystem::is_directory(response_target))
	{
		std::cout << "----------------------------------------------------XXXXXXX " << response_target << std::endl;
		status_code = 301;
		location = response_target.substr(location_config->root.size()); // response target without root
		location = "http://" + request.host + ':' + std::to_string(port) + location + '/';
		// location = "http://" + request.host + ':' + std::to_string(port) + request.request_target + '/';
		
		normalize_path(location);
		std::cout << "----------------------------------------------------location " << location << std::endl;
		return ;
	}

	if (!std::filesystem::exists(response_target))
	{
		std::cout << "-------------------------------------++x++ " << response_target << std::endl;
		status_code = 404;
		return ;
	}

	ifs_body = std::make_shared<std::ifstream>(response_target, std::ios::binary);
	if (!ifs_body->is_open())
	{
		status_code = 403;
		ifs_body.reset();
	}
}

void	Response::init_error_body(int& status_code, const Request& request, const uint16_t port)
{
	if (server_config->error_page.find(status_code) != server_config->error_page.end())
	{
		response_target = server_config->error_page.at(status_code);
		if (response_target.front() == '/')
		{
			set_location_config(response_target);
			set_response_target(response_target, status_code);
			init_body(status_code, request, port);
		}
		else
		{
			status_code = 302;
			location = response_target;
		}
	}

	if (str_body.empty() && !ifs_body)
	{
		set_status_text(status_code);
		generate_error_page(status_code);
	}
}

// nginx checks if request_target.back is '/'.
// if it is not, then it will check if it is a directory.
// if it is a directory, it will send 301 moved permanently
// Location: http://<request.host>:<server.port>/<request_target>/
// the '/' at the end indicating that it is a directory, not a file

// void	Response::set_body_path(int& status_code, std::string& request_target, const Request& request, const Connection& connection)
// {
// 	if (status_code >= 300)
// 	{
// 		auto it = location_config->error_page.find(status_code);
// 		if (it != location_config->error_page.end())
// 			request_target = it->second;
// 		else
// 			return ;
// 	}
// 	body_path = location_config->root + request_target;
// 	// if body_path[0] != '/'
// 	// 		body_path = PWD + body_path;


// 	bool directory_request = (request_target.back() == '/');
// 	std::cout << "-----------------------------------------------------------------------" << directory_request << std::endl;
	
// 	std::cout << "XXXXXXXXXXXX body path: " << body_path << std::endl;

// 	if (!directory_request && std::filesystem::is_directory(body_path))
// 	{
// 		status_code = 301;
// 		location = "http://" + request.host + ':' + std::to_string(connection.server->port) + request_target + '/';
// 		std::cout << "XXXXXXXXXXXX setting location: " << location << std::endl;
// 		return ;
// 	}



// 	if (directory_request)
// 	{
// 		if (std::filesystem::exists(body_path + location_config->index))
// 		{
// 			body_path += location_config->index;
// 			// 302
// 			directory_request = (body_path.back() == '/');
// 			std::cout << "--------------------------------------------a" << std::endl;
// 		}
// 		else if (location_config->autoindex)
// 		{
// 			directory_listing = true;
// 			std::cout << "--------------------------------------------b" << std::endl;
// 		}
// 		else
// 		{
// 			status_code = 404;
// 			std::cout << "--------------------------------------------c" << std::endl;
// 		}
// 	}



	// if (directory_request && std::filesystem::exists(body_path + location_config->index))
	// {
	// 	body_path += location_config->index;
	// 	bool directory_request = (body_path.back() == '/');
	// }

	// if (!std::filesystem::exists(body_path))
	// {
	// 	if (directory_request)
	// 	{
	// 		if (location_config->autoindex)
	// 			directory_listing = true;
	// 		else
	// 			status_code = 403; // directory index of <body_path> is forbidden
	// 		return ;
	// 	}
	// 	status_code = 404;
	// }




	// std::ifstream file(body_path);
	// if (!file)
	// 	status_code = 403;
	// if (!directory_request)
	// {
	// 	ifs_body = std::make_shared<std::ifstream>(body_path, std::ios::binary);
	// 	if (!ifs_body->is_open())
	// 	{
	// 		status_code = 404;
	// 		ifs_body.reset();
	// 	}
		// if (ifs_body->fail())
		// {
		// 	status_code = 403;
		// 	ifs_body.reset();
		// }
// 	}
// }

static std::string format_date_time(const std::filesystem::file_time_type& ftime)
{
	auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
	std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
	std::tm tm = *std::gmtime(&cftime);

	std::ostringstream oss;
	oss << std::put_time(&tm, "%d-%b-%Y %H:%M");
	return (oss.str());
}

void	Response::generate_directory_listing(const Request& request, const uint16_t port)
{
	(void)port;
	std::cout << "generating directory listing" << std::endl;
	str_body = "<html><head><meta charset=\"UTF-8\"><title>directory_listing_placeholder directory_listing_placeholder</title></head><body><center><h1>directory_listing_placeholder directory_listing_placeholder</h1></center><hr><center>🐢webserv🐢</center></body></html>\n";
	
	// std::string dir = body_path;
	std::ostringstream oss_body;
	oss_body	<< "<html>" << "\r\n"
				<< "<head><title>Index of " << request.request_target << "</title></head>" << "\r\n"
				<< "<body>" << "\r\n"
				<< "<h1>Index of " << request.request_target << "</h1><hr><pre><a href=\"../\">../</a>" << "\r\n";

	for (const auto& i : std::filesystem::directory_iterator(response_target))
	{
		if (!i.is_directory())
			continue;
		std::string filename = i.path().filename().string() + '/';
		// std::string link = request.request_target.substr(1) + filename;
		std::string link = filename;
		// link.erase(link.begin());
		if (filename.length() > 50)
		{
			filename.resize(47);
			filename += "..>";
		}
		std::string date_time = format_date_time(i.last_write_time());
		std::string filesize = "-";
		std::string s1(51 - filename.size(), ' ');

		oss_body	<< "<a href=\"" << link << "\">" << filename << "</a>" << s1 << date_time << ' ' << std::right << std::setw(19) << filesize << "\r\n";
	}
	for (const auto& i : std::filesystem::directory_iterator(response_target))
	{
		if (i.is_directory())
			continue;
		std::string filename = i.path().filename().string();
		// std::string link = request.request_target.substr(1) + filename;
		std::string link = filename;
		// link.erase(link.begin());
		if (filename.length() > 50)
		{
			filename.resize(47);
			filename += "..>";
		}
		std::string date_time = format_date_time(i.last_write_time());
		std::string filesize = std::to_string(i.file_size());
		std::string s1(51 - filename.size(), ' ');
		oss_body	<< "<a href=\"" << link << "\">" << filename << "</a>" << s1 << date_time << ' ' << std::right << std::setw(19) << filesize << "\r\n";
	}

	oss_body	<< "</pre><hr></body>\r\n</html>\r\n";
	
	str_body = oss_body.str();
	content_length = std::to_string(str_body.length());
	// std::cout << str_body << std::endl;

	transfer_encoding = "chunked";
	std::ostringstream oss;
	oss << std::hex << str_body.length();
	std::string chunk_size = oss.str() + "\r\n";

	str_body.insert(str_body.begin(), chunk_size.begin(), chunk_size.end());
	str_body.append("\r\n0\r\n\r\n");
}

void	Response::generate_error_page(const int status_code)
{
	std::cout << "generating error page" << std::endl;
	// static const std::string error_page_template = 
	str_body = "<html>\r\n<head><meta charset=\"UTF-8\"><title>" + std::to_string(status_code) + " " + status_text + "</title></head>\r\n<body>\r\n<center><h1>" + std::to_string(status_code) + " " + status_text + "</h1></center>\r\n<hr><center>🐢webserv🐢</center>\r\n</body>\r\n</html>\r\n";
	// str_body = "<html>\r\n<head><title>" + std::to_string(status_code) + " " + status_text + "</title></head>\r\n<body>\r\n<center><h1>" + std::to_string(status_code) + " " + status_text + "</h1></center>\r\n<hr><center>nginx/1.27.4</center>\r\n</body>\r\n</html>\r\n";
	// std::cout << str_body << std::endl;
	content_length = std::to_string(str_body.length());
}

void	Response::set_status_text(const int status_code)
{
	static const std::map<int, std::string> status_map =
	{
		{100, "Continue"},
		{101, "Switching Protocols"},
		{102, "Processing Deprecated"},
		{103, "Early Hints"},
		{200, "OK"},
		{201, "Created"},
		{202, "Accepted"},
		{203, "Non-Authoritative Information"},
		{204, "No Content"},
		{205, "Reset Content"},
		{206, "Partial Content"},
		{207, "Multi-Status (WebDAV)"},
		{208, "Already Reported (WebDAV)"},
		{226, "IM Used (HTTP Delta encoding)"},
		{300, "Multiple Choices"},
		{301, "Moved Permanently"},
		{302, "Found"},
		{303, "See Other"},
		{304, "Not Modified"},
		{305, "Use Proxy Deprecated"},
		{306, "unused"},
		{307, "Temporary Redirect"},
		{308, "Permanent Redirect"},
		{400, "Bad Request"},
		{401, "Unauthorized"},
		{402, "Payment Required"},
		{403, "Forbidden"},
		{404, "Not Found"},
		{405, "Method Not Allowed"},
		{406, "Not Acceptable"},
		{407, "Proxy Authentication Required"},
		{408, "Request Timeout"},
		{409, "Conflict"},
		{410, "Gone"},
		{411, "Length Required"},
		{412, "Precondition Failed"},
		{413, "Content Too Large"},
		{414, "URI Too Long"},
		{415, "Unsupported Media Type"},
		{416, "Range Not Satisfiable"},
		{417, "Expectation Failed"},
		{418, "I'm a teapot"},
		{421, "Misdirected Request"},
		{422, "Unprocessable Content (WebDAV)"},
		{423, "Locked (WebDAV)"},
		{424, "Failed Dependency (WebDAV)"},
		{425, "Too Early Experimental"},
		{426, "Upgrade Required"},
		{428, "Precondition Required"},
		{429, "Too Many Requests"},
		{431, "Request Header Fields Too Large"},
		{451, "Unavailable For Legal Reasons"},
		{500, "Internal Server Error"},
		{501, "Not Implemented"},
		{502, "Bad Gateway"},
		{503, "Service Unavailable"},
		{504, "Gateway Timeout"},
		{505, "HTTP Version Not Supported"},
		{506, "Variant Also Negotiates"},
		{507, "Insufficient Storage (WebDAV)"},
		{508, "Loop Detected (WebDAV)"},
		{510, "Not Extended"},
		{511, "Network Authentication Required"}
	};

	status_text = status_map.at(status_code);
}

void	Response::set_content_type(void)
{
	static const std::map<std::string, std::string> type_map =
	{
		{"png"	, "image/png"},
		{"jpg"	, "image/jpeg"},
		{"jpeg"	, "image/jpeg"},
		{"jpe"	, "image/jpeg"},
		{"gif"	, "image/gif"},
		{"webp"	, "image/webp"},
		{"svg"	, "image/svg+xml"},
		{"bmp"	, "image/bmp"},
		{"tiff"	, "image/tiff"},
		{"tif"	, "image/tiff"},
		{"ico"	, "image/vnd.microsoft.icon"},
		{"ico"	, "image/x-icon"},
		{"heif"	, "image/heif"},
		{"heic"	, "image/heif"},
		{"avif"	, "image/avif"},

		{"midi"	, "audio/midi"},
		{"mid"	, "audio/midi"},
		{"mp3"	, "audio/mpeg"},
		{"ogg"	, "audio/ogg"},
		{"oga"	, "audio/ogg"},
		{"wav"	, "audio/wav"},
		{"aac"	, "audio/x-aac"},

		{"mp4"	, "video/mp4"},
		{"avi"	, "video/x-msvideo"},
		{"flv"	, "video/x-flv"},
		{"mkv"	, "video/x-matroska"},
		{"ogv"	, "video/ogg"},
		{"webm"	, "video/webm"},
		{"mov"	, "video/quicktime"},

		{"css"	, "text/css"},
		{"csv"	, "text/csv"},
		{"htm"	, "text/html"},
		{"html"	, "text/html"},
		{"js"	, "text/javascript"},
		{"txt"	, "text/plain"},
		{"xml"	, "text/xml"},

		{"atom"	, "application/atom+xml"},
		{"json"	, "application/json"},
		{"js"	, "application/javascript"},
		{"pdf"	, "application/pdf"},
		{"xls"	, "application/vnd.ms-excel"},
		{"xlsx"	, "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
		{"ppt"	, "application/vnd.ms-powerpoint"},
		{"pptx"	, "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
		{"url"	, "application/x-www-form-urlencoded"},
		{"zip"	, "application/zip"},
		{"rar"	, "application/x-rar-compressed"},
		{"gz"	, "application/x-gzip"}
	};

	std::smatch match;
	if (!ifs_body)
		content_type = "text/html";
	else if (std::regex_match(response_target, match, std::regex(R"(.*\.(\w*))")))
	{
		std::string file_extension = match[1];
		auto it = type_map.find(file_extension);
		if (it != type_map.end())
			content_type = it->second;
	}
}
