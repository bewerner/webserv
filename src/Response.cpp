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
	if (match_length == 0)
		throw std::logic_error("this should never happen. investigate (in set_location_config function). request_target: " + request_target);
}

void Response::extract_path_info(std::string& request_target)
{
	path_info = request_target;
	request_target.clear();
	std::smatch match;

	while (std::regex_match(path_info, match, std::regex(R"((/+[^/]*)(.*))")))
	{
		request_target.append(match[1]);
		path_info = match[2];
		std::cout << "cgi target: >" << request_target << "<   path_info: >" << path_info << "<" << std::endl;
		if (!std::filesystem::exists(location_config->root + request_target) || !std::filesystem::is_directory(location_config->root + request_target))
			break ;
	}
}

void	Response::set_response_target(std::string request_target, int& status_code, std::string method)
{
	const LocationConfig* config = location_config;

	if (!config->alias.empty())
		request_target.erase(0, config->path.size());
	if (method == "DELETE")
	{
		response_target = config->root + request_target;
		return ;
	}

	bool directory_request = (request_target.back() == '/');
	bool absolute_index    = (config->index.front() == '/');
	bool directory_index   = (config->index.back()  == '/');

	for (size_t i = 0; directory_request && absolute_index && directory_index; i++) // request target and root are irrelevant -> jump to location matching index with index as request target
	{
		if (i > 9)
		{
			status_code = 500;
			return ;
		}
		set_location_config(config->index);
		config = location_config;
		request_target = config->index;
		directory_request = (request_target.back() == '/');
		absolute_index    = (config->index.front() == '/');
		directory_index   = (config->index.back()  == '/');
		if (directory_request)
			request_target.clear();
	}

	// if (config.cgi)
	if (location_config->cgi)
		extract_path_info(request_target);

	if (directory_request)
	{
		if (absolute_index) // request target is irrelevant -> search for root+index
		{
			response_target = config->index;
			// sleep(1); std::cout << response_target << std::endl; sleep (1);
			set_location_config(response_target);
			config = location_config;
			if (!config->alias.empty())
				response_target.erase(0, config->path.size());
			// sleep(1); std::cout << response_target << std::endl; sleep (1);
			response_target = config->root + response_target;
			// sleep(1); std::cout << response_target << std::endl; sleep (1);
			// if (!std::filesystem::exists(response_target))
			// 	response_target = config->root;
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

void	Response::init_cgi(int& status_code, char** envp, const Server& server, const Request& request, const Response& response)
{
	(void)status_code;
	cgi.init_pipes();
	cgi.fork();
	cgi.setup_io();
	cgi.exec(envp, server, request, response);
}

void	Response::init_body(int& status_code, const Request& request, const Response& response, const Server& server, char** envp)
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
			generate_directory_listing(request);
		else
			status_code = 403;
		return ;
	}

	if (std::filesystem::is_directory(response_target))
	{
		std::cout << "----------------------------------------------------XXXXXXX " << response_target << std::endl;
		status_code = 301;
		// location = response_target.substr(location_config->root.size()); // response target without root
		location = request.request_target;
		if (location.back() == '/')
			location.append(location_config->index);
		location = "http://" + request.host + ':' + std::to_string(server.port) + location + '/';
		// location = "http://" + request.host + ':' + std::to_string(server.port) + request.request_target + '/';
		
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

	// if (location_config->cgi)
	std::cout << "----------------------------------------------------------------------------------------------------" << request.request_target << std::endl;
	if (location_config->cgi)
	{
	std::cout << "----------------------------------------------------------------------------------------------------" << request.request_target << std::endl;
		init_cgi(status_code, envp, server, request, response);
		if (cgi.fail)
		{
			status_code = 500;
			cgi = CGI();
		}
		return;
	}

	ifs_body = std::make_shared<std::ifstream>(response_target, std::ios::binary);
	if (!ifs_body->is_open())
	{
		status_code = 403;
		ifs_body.reset();
	}
}

void	Response::init_error_body(int& status_code, const Request& request, const Server& server, char** envp)
{
	if (server_config->error_page.find(status_code) != server_config->error_page.end())
	{
		response_target = server_config->error_page.at(status_code);
		if (response_target.front() == '/')
		{
			set_location_config(response_target);
			set_response_target(response_target, status_code, "GET");
			init_body(status_code, request, *this, server, envp);
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


static std::string get_date()
{
	std::time_t now = std::time(nullptr);
	std::tm gmt_tm = *std::gmtime(&now);

	std::ostringstream date_stream;
	date_stream << std::put_time(&gmt_tm, "%a, %d %b %Y %H:%M:%S GMT");
	return (date_stream.str());
}

void	Response::create_header(const int status_code)
{
	std::ostringstream oss;
	oss			<< "HTTP/1.1 "				<< status_code << ' ' << status_text		<< "\r\n"
				<< "Server: "				<< server									<< "\r\n"
				<< "Date: "					<< get_date()								<< "\r\n"
				<< "Content-Type: "			<< content_type								<< "\r\n";
	if (transfer_encoding.empty())
		oss		<< "Content-Length: "		<< content_length							<< "\r\n";
	else
		oss		<< "Transfer-Encoding: "	<< transfer_encoding						<< "\r\n";
	if (!location.empty() && location.find("http://") == 0)
		oss		<< "Location: "				<< location									<< "\r\n";
	oss			<< "Connection: "			<< connection								<< "\r\n";
	if (!location.empty() && location.find("http://") != 0)
		oss		<< "Location: "				<< location									<< "\r\n";
	oss			<< "\r\n";

	header = oss.str();
	header_sent = false;
}

static std::string format_date_time(const std::filesystem::file_time_type& ftime)
{
	auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
	std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
	std::tm tm = *std::gmtime(&cftime);

	std::ostringstream oss;
	oss << std::put_time(&tm, "%d-%b-%Y %H:%M");
	return (oss.str());
}

static std::string	entry_listing(const std::filesystem::directory_entry& entry, const bool is_directory)
{
	std::string filename = entry.path().filename().string();
	if (is_directory)
		filename.append("/");
	std::string link = filename;
	size_t max_length = 50;
	if (filename.length() > max_length)
	{
		filename.resize(max_length - 3);
		filename.append("..&gt;");
		max_length += 3;
	}
	std::string date_time = format_date_time(entry.last_write_time());
	std::string filesize("-");
	if (entry.is_regular_file())
		filesize = std::to_string(entry.file_size());
	std::string padding(max_length - filename.size(), ' ');
	std::ostringstream oss;
	oss << "<a href=\"" << link << "\">" << filename << "</a>" << padding << ' '<< date_time << ' ' << std::right << std::setw(19) << filesize;
	return (oss.str());
}

void	Response::extract_cgi_header(std::array<char, BUFFER_SIZE>& buf, ssize_t& size, int& status_code)
{
	cgi.header.insert(cgi.header.end(), buf.begin(), buf.begin() + size);
	std::smatch match;
	if (std::regex_match(cgi.header, match, std::regex(R"(([\s\S]*?\n)(\r?\n)([\s\S]*))")))
	{
		cgi.header_extracted = true;
		std::string body = match[3];
		cgi.header = match[1];
		// debug
		std::cout	<< "--------------------cgiRESPONSE-HEADER--------------------\n"
					<< cgi.header
					<< "------------------------------------------------------\n\n\n" << std::endl;

		for (size_t i = 0; i < body.length(); i++)
			buf[i] = body[i];
		size = body.length();

		//validate cgi header
		if (!std::regex_match(cgi.header, std::regex(R"((\s*?\S+:.*\r?\n)+|\s*)", std::regex_constants::icase)))
		{
			cgi.fail = true;
			status_code = 500;
			status_text = "Internal Server Error";
			generate_error_page(status_code);
			connection = "close";
			create_header(status_code);
			std::cout << "xxxxxxxxxxxxxxxxxxxxWRONGxCGIxHEADERxFORMAT" << std::endl;
			return ;
		}
		//parse cgi header
		if (std::regex_search(cgi.header, match, std::regex(R"(^\s*Content-Type:.*?(\S+.*))", std::regex_constants::icase)))
		{
			content_type = match[1];
			std::cout << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx found content type: " << content_type << std::endl;
		}

		//update response header
		create_header(status_code);
		// debug
		std::cout	<< "--------------------RESPONSE-HEADERx--------------------\n"
					<< header
					<< "------------------------------------------------------\n\n\n" << std::endl;
		// if (cgi.fail)
		// 	exit(0);
	}
	else
		size = 0;
}

void	Response::generate_directory_listing(const Request& request)
{
	std::cout << "generating directory listing" << std::endl;

	std::map<std::string, std::filesystem::directory_entry> directories;
	std::map<std::string, std::filesystem::directory_entry> non_directories;
	
	std::ostringstream oss;
	oss	<< "<html>"																					<< "\r\n"
		<< "<head><title>Index of " << request.request_target << "</title></head>"					<< "\r\n"
		<< "<body>"																					<< "\r\n"
		<< "<h1>Index of " << request.request_target << "</h1><hr><pre><a href=\"../\">../</a>"		<< "\r\n";
	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(response_target))
	{
		if (entry.is_directory())
			directories.insert(std::make_pair(entry.path().filename().string(), entry));
		else
			non_directories.insert(std::make_pair(entry.path().filename().string(), entry));
	}
	for (const auto& [filename, entry] : directories)
		oss << entry_listing(entry, true)															<< "\r\n";
	for (const auto& [filename, entry] : non_directories)
		oss << entry_listing(entry, false)															<< "\r\n";
	oss	<< "</pre><hr></body>\r\n</html>"															<< "\r\n";
	str_body = oss.str();

	transfer_encoding = "chunked";
	std::string chunk_size = (std::ostringstream{} << std::hex << str_body.length() << "\r\n").str();
	
	str_body.insert(0, chunk_size);
	str_body.append("\r\n0\r\n\r\n");
}

void	Response::generate_error_page(const int status_code)
{
	std::cout << "generating error page" << std::endl;

	str_body = "<html>\r\n<meta charset=\"UTF-8\"><!--🐢-->\r\n<head><title>" + std::to_string(status_code) + " " + status_text + "</title></head>\r\n<body>\r\n<center><h1>" + std::to_string(status_code) + " " + status_text + "</h1></center>\r\n<hr><center>🐢webserv🐢</center>\r\n</body>\r\n</html>\r\n";
	std::ostringstream oss;
	oss	<<	"<html>"																			<<	"\r\n"
		<<	"<meta charset=\"UTF-8\"><!--🐢-->"													<<	"\r\n"
		<<	"<head><title>"	<<	status_code	<<	' '	<<	status_text	<<	"</title></head>"		<<	"\r\n"
		<<	"<body>"																			<<	"\r\n"
		<<	"<center><h1>"	<<	status_code	<<	' '	<<	status_text	<<	"</h1></center>"		<<	"\r\n"
		<<	"<hr><center>"	<<	"🐢webserv🐢"	<<	"</center>"									<<	"\r\n"
		<<	"</body>"																			<<	"\r\n"
		<<	"</html>"																			<<	"\r\n";

	str_body = oss.str();
	content_length = std::to_string(str_body.length());
	transfer_encoding.clear();
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
		{414, "Request-URI Too Large"},
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
