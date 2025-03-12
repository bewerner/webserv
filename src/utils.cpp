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
