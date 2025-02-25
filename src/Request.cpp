#include "webserv.hpp"

void	trim_whitespaces(std::string& str)
{
	size_t	start = str.find_first_not_of(" \t\n\r\f\v");
	size_t	end = str.find_last_not_of(" \t\n\r\f\v");

	std::string	new_str = str.substr(start, end + 1 - start);
	str = new_str;
}

bool	parse_start_line(Request& request , std::istringstream& iss_header)
{
	std::string	start_line;
	std::getline(iss_header, start_line);

	// std::regex	pattern(R"([A-Z]+\s+\S+\s+HTTP/\d+\.\d{1,3}\s*)");
	std::regex	pattern(R"([A-Z]+\s+\/\S*\s+HTTP\/\d+\.?\d{0,3}\s*)");
	if (!std::regex_match(start_line, pattern))
	{
		request.status_code = 400;
		return (false);
	}

	std::istringstream	istream(start_line);
	std::string	method;
	std::string	request_target;
	std::string	protocol;

	istream >> method >> request_target >> protocol;
	if (method != "GET" && method != "POST" && method != "DELETE")
	{
		request.status_code = 405;
		return (false);
	}
	request.method = method;

	request.request_target = request_target;

	// size_t	pos = start_line.find("HTTP");
	// if (start_line[pos + 4] != '1' && start_line[pos + 5] != '.')
	// {
	// 	request.status_code = 505;
	// 	return (false);
	// }
	if (protocol.find("HTTP/1.") == std::string::npos)
	{
		request.status_code = 505;
		return (false);
	}
	request.protocol = "HTTP/1.1";

	return (true);
}

bool	parse_header(Request& request, std::string& key, std::string& value)
{
	if (key == "host")
	{
		if (request.host.length() != 0)
		{
			request.status_code = 400;
			return (false);
		}
		request.host = value;
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
			request.content_length = std::stoul(key);
			// if (value_length > std::numeric_limits<unsigned int>::max())
				// throw std::out_of_range("Value is greater than max unsigned int");
			// request.content_length = static_cast<unsigned int>(value_length);
		}
		catch (std::invalid_argument &e)
		{
			request.status_code = 400;
			return (false);
		}
		catch (std::out_of_range &e)
		{
			request.status_code = 413;
			return (false);
		}
	}
	return (true);
}

void	parse_request(Request& request)
{
	std::istringstream iss_header(request.header);

	if (!parse_start_line(request, iss_header))
		return ;

	std::string	line;
	while (std::getline(iss_header, line))
	{
		std::regex	pattern(R"(\S+:\s*\S+)");
		if (!std::regex_match(line, pattern))
			continue ;

		size_t	pos = line.find(":");
		std::string	key = line.substr(0, pos);
		std::string	value = line.substr(pos + 1);

		std::transform(key.begin(), key.end(), key.begin(), ::tolower);
		std::transform(value.begin(), value.end(), value.begin(), ::tolower);
		trim_whitespaces(value);

		if (!parse_header(request, key, value))
			return ;
	}
	if (request.host.length() == 0)
		request.status_code = 400;
}
