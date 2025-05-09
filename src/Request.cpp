#include "webserv.hpp"

static void	decode_URI(std::string& URI)
{
	size_t pos;
	if ((pos = URI.find("#")) != std::string::npos)
		URI.resize(pos);
	while ((pos = URI.find("%20")) != std::string::npos)
		URI.replace(pos, 3, " ");
	while ((pos = URI.find("%2F")) != std::string::npos)
		URI.replace(pos, 3, "/");
}

bool	parse_start_line(Request& request , std::istringstream& iss_header, int& status_code)
{
	std::string	start_line;
	std::getline(iss_header, start_line);

	if (start_line.length() > 8191)
	{
		status_code = 414;
		return (false);
	}

	std::regex	pattern(R"([A-Z]+ +\/\S* +HTTP\/\d+\.?\d{0,3} *)");
	if (!std::regex_match(start_line, pattern))
	{
		std::cout << "------------------------------pattern mismatch" << std::endl;
		status_code = 400;
		return (false);
	}

	std::istringstream	istream(start_line);
	std::string	method;
	std::string	request_target;
	std::string	protocol;

	istream >> method >> request_target >> protocol;

	request.method = method;

	request.URI = request_target;
	request.request_target = request_target;
	decode_URI(request.request_target);
	normalize_path(request.request_target);
	if (!collapse_absolute_path(request.request_target))
	{
		std::cout << "------------------------------request target above root" << std::endl;
		status_code = 400;
		return (false);
	}

	if (protocol.find("HTTP/1.") == std::string::npos)
	{
		status_code = 505;
		return (false);
	}
	request.protocol = "HTTP/1.1";

	return (true);
}

static void	normalize_host(std::string& host, std::string& port) // removes ':port' portion from host if present
{
	size_t end = host.find(':');
	if (end != std::string::npos)
	{
		port = host.substr(end + 1);
		host.resize(end);
	}
}

bool	parse_header(Request& request, std::string& key, std::string& value, int& status_code)
{
	if (key == "host")
	{
		request.host = value;
		normalize_host(request.host, request.port);
	}
	if (key == "user-agent")
	{
		request.user_agent = value;
	}
	if (key == "referer")
	{
		request.referer = value;
	}
	if (key == "origin")
	{
		request.origin = value;
	}
	else if (key == "connection")
	{
		if (value == "close" || value == "keep-alive")
			request.connection = value;
	}
	else if (key == "content-type")
	{
		request.content_type = value;
	}
	else if (key == "transfer-encoding")
	{
		if (value != "chunked")
		{
			status_code = 501;
			return (false);
		}
		request.transfer_encoding = value;
	}
	else if (key == "content-length")
	{
		request.content_length_specified = true;
		try
		{
			if (!std::all_of(value.begin(), value.end(), ::isdigit))
				throw std::invalid_argument("Value can only contain digits");
			request.content_length = std::stoul(value);
			request.remaining_bytes = request.content_length;
		}
		catch (std::invalid_argument &e)
		{
		std::cout << "------------------------------bbb " << e.what() << '>' << value << '<' << std::endl;
			status_code = 400;
			return (false);
		}
		catch (std::out_of_range &e)
		{
			status_code = 413;
			return (false);
		}
	}
	return (true);
}

void	parse_request(Request& request, int& status_code)
{
	std::istringstream iss_header(request.header);

	if (!parse_start_line(request, iss_header, status_code))
		return ;

	std::string	line;
	while (std::getline(iss_header, line))
	{
		std::smatch match;
		std::regex	pattern(R"((\S+):\s*(\S+.*?)\s*)");
		if (!std::regex_match(line, match, pattern))
		{
			std::cout << "------------------------------MISMATCH: >" << line << "<" << std::endl;
			continue ;
		}

		std::string key = match[1];
		std::string value = match[2];

		std::transform(key.begin(), key.end(), key.begin(), ::tolower);
		if (key != "content-type")
			std::transform(value.begin(), value.end(), value.begin(), ::tolower);

		if (!parse_header(request, key, value, status_code))
			return ;
	}
	if (request.host.empty())
	{
		std::cout << "------------------------------c" << std::endl;
		status_code = 400;
	}
	if (request.content_length_specified && !request.transfer_encoding.empty())
		status_code = 501;
}
