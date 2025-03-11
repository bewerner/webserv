#include "webserv.hpp"

// void	trim_whitespaces(std::string& str)
// {
// 	size_t	start = str.find_first_not_of(" \t\n\r\f\v");
// 	size_t	end = str.find_last_not_of(" \t\n\r\f\v");

// 	std::string	new_str = str.substr(start, end + 1 - start);
// 	str = new_str;
// }

bool	parse_start_line(Request& request , std::istringstream& iss_header, int& status_code)
{
	std::string	start_line;
	std::getline(iss_header, start_line);

	// std::regex	pattern(R"([A-Z]+\s+\S+\s+HTTP/\d+\.\d{1,3}\s*)");
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
	if (method != "GET" && method != "POST" && method != "DELETE")
	{
		status_code = 405;
		return (false);
	}
	request.method = method;

	request.request_target = request_target;
	normalize_path(request.request_target);

	// size_t	pos = start_line.find("HTTP");
	// if (start_line[pos + 4] != '1' && start_line[pos + 5] != '.')
	// {
	// 	status_code = 505;
	// 	return (false);
	// }
	if (protocol.find("HTTP/1.") == std::string::npos)
	{
		status_code = 505;
		return (false);
	}
	request.protocol = "HTTP/1.1";

	return (true);
}

static void	normalize_host(std::string& host) // removes ':port' portion from host if present
{
	size_t end = host.find(':');
	if (end != std::string::npos)
		host.resize(end);
}

bool	parse_header(Request& request, std::string& key, std::string& value, int& status_code)
{
	if (key == "host")
	{
		request.host = value;
		normalize_host(request.host);
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
	else if (key == "content-length")
	{
		try
		{
			if (!std::all_of(value.begin(), value.end(), ::isdigit))
				throw std::invalid_argument("Value can only contain digits");
			request.content_length = std::stoul(value);
			// if (value_length > std::numeric_limits<unsigned int>::max())
				// throw std::out_of_range("Value is greater than max unsigned int");
			// request.content_length = static_cast<unsigned int>(value_length);
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
		// std::regex	pattern(R"(\S+:\s*(\S+\s*)+)");
		std::smatch match;
		std::regex	pattern(R"((\S+):\s*(\S+).*)");
		if (!std::regex_match(line, match, pattern))
		{
			std::cout << "------------------------------MISMATCH" << std::endl;
			continue ;
		}

		// size_t	pos = line.find(":");
		// std::string	key = line.substr(0, pos);
		// std::string	value = line.substr(pos + 1);
		std::string key = match[1];
		std::string value = match[2];

		std::transform(key.begin(), key.end(), key.begin(), ::tolower);
		std::transform(value.begin(), value.end(), value.begin(), ::tolower);
		// trim_whitespaces(value);

		if (!parse_header(request, key, value, status_code))
			return ;
	}
	if (request.host.empty())
	{
		std::cout << "------------------------------c" << std::endl;
		status_code = 400;
	}
}
