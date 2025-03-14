#include "webserv.hpp"

void	normalize_path(std::string& path)
{
	size_t pos;
	while ((pos = path.find("//")) != std::string::npos)
	{
		path.erase(path.begin() + pos);
	}
	if (path.find("http:/") == 0)
		path.insert(path.begin() + 5, '/');
	else if (path.find("https:/") == 0)
		path.insert(path.begin() + 6, '/');
}

bool	collapse_absolute_path(std::string& path)
{
	if (path.front() != '/')
		return (false);

	// if path ends with "/.." or "/." append '/' (directory request)
	if (std::regex_match(path, std::regex(R"(.*/\.\.?)")))
		path.append("/");

	size_t pos;
	while ((pos = path.find("/./")) != std::string::npos)
		path.erase(pos, 2);
	while ((pos = path.find("/../")) != std::string::npos)
	{
		// if path starts with "/../" it goes above root -> invalid (bad request)
		if (pos == 0)
			return (false);
		// collapse the backtrack
		size_t end = pos + 3;
		size_t start = pos - 1;
		while (start > 0 && path[start] != '/')
			start--;
		path.erase(path.begin() + start, path.begin() + end);
	}

	return (true);
}
