#include "webserv.hpp"

void	trim_whitespaces(std::string& str)
{
	size_t	start = str.find_first_not_of(" \t\n\r\f\v");
	size_t	end = str.find_last_not_of(" \t\n\r\f\v");

	std::string	new_str = str.substr(start, end + 1 - start);
	str = new_str;
}

bool	parse_start_line(Connection& connection, std::istringstream& request)
{
	std::string	start_line;
	std::getline(request, start_line);

	std::regex	pattern(R"([A-Z]+\s+\S+\s+HTTP/\d+\.\d{1,3}\s*)");
	if (!std::regex_match(start_line, pattern))
	{
		connection.request.status_code = 400;
		return (false);
	}

	std::istringstream	istream(start_line);
	std::string	method;
	std::string	request_target;
	std::string	protocol;

	istream >> method >> request_target >> protocol;
	if (method != "GET" && method != "POST" && method != "DELETE")
	{
		connection.request.status_code = 405;
		return (false);
	}
	connection.request.method = method;

	connection.request.request_target = request_target;

	size_t	pos = start_line.find("HTTP");
	if (start_line[pos + 4] != '1' && start_line[pos + 5] != '.')
	{
		connection.request.status_code = 505;
		return (false);
	}
	connection.request.protocol = "HTTP/1.1";

	return (true);
}

bool	parse_header(Connection& connection, std::string& key, std::string& value)
{
	if (key == "host")
	{
		if (connection.request.host.length() != 0)
		{
			connection.request.status_code = 400;
			return (false);
		}
		connection.request.host = value;
	}
	else if (key == "connection")
	{
		if (value == "close" || value == "keep-alive")
			connection.request.connection = value;
	}
	else if (key == "content-type")
	{
		connection.request.content_type = value;
	}
	else if (key == "content-length")
	{
		try
		{
			if (!std::all_of(value.begin(), value.end(), ::isdigit))
				throw std::invalid_argument("Value can only contain digits");
			connection.request.content_length = std::stoul(key);
			// if (value_length > std::numeric_limits<unsigned int>::max())
				// throw std::out_of_range("Value is greater than max unsigned int");
			// connection.request.content_length = static_cast<unsigned int>(value_length);
		}
		catch (std::invalid_argument &e)
		{
			connection.request.status_code = 400;
			return (false);
		}
		catch (std::out_of_range &e)
		{
			connection.request.status_code = 413;
			return (false);
		}
	}
	return (true);
}

void	parse_request(Connection& connection, std::string& request_header)
{
	// only request as input, instead of connection
	std::istringstream request(request_header);

	if (!parse_start_line(connection, request))
		return ;

	std::string	line;
	while (std::getline(request, line))
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

		if (!parse_header(connection, key, value))
			return ;
	}
	if (connection.request.host.length() == 0)
		connection.request.status_code = 400;
}
