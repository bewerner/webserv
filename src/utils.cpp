#include "webserv.hpp"

void	normalize_path(std::string& path)
{
	size_t pos;
	while ((pos = path.find("//")) != std::string::npos)
	{
		path.erase(path.begin() + pos);
	}
}
