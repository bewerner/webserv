#include "webserv.hpp"

void	Response::set_body_path(int& status_code, const std::string& request_target)
{
	if (status_code >= 400)
		body_path = "html/error/" + std::to_string(status_code) + ".html";
	else if (request_target.find('.') != std::string::npos)
		body_path = "html" + request_target;
	else
		body_path = "html/index.html";

	if (!std::filesystem::exists(body_path))
	{
		status_code = 404;
		body_path = "html/error/404.html";
	}
	std::ifstream file(body_path);
	if (!file)
	{
		status_code = 403;
		body_path = "html/error/403.html";
	}
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
