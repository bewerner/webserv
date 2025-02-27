#include "webserv.hpp"

bool isValidLocationKey(const std::string& key)
{
	static const std::set<std::string> validKeys =
	{
		"root",
		"autoindex",
		"index",
		"upload_dir",
		"cgi_extension",
		"allow_methods",
		"client_max_body_size"
	};
	return validKeys.find(key) != validKeys.end();
}

bool isValidServerKey(const std::string& key)
{
	static const std::set<std::string> validKeys =
	{
		"listen",
		"server_name",
		"root",
		"index",
		"client_max_body_size",
		"error_page"
	};
	return validKeys.find(key) != validKeys.end();
}

void validateConfigurations(const std::vector<Server>& servers) {
	(void)servers;
	// Locations Validate TODO
	// ValidatePath(servers);
	// ValidateRoot(servers);
	// ValidateDirectives(servers);
	// ValidateAllowMethos(servers);
	// ValidateAutoIndex(servers);
	// ValidateIndex(servers);
	// ValidateClientBodyTempPath(servers);
	// ValidateFastcgiParam(servers);
	// ValidateLocationClientMaxBodySize(servers);

	// Server Validate TODO
	// ValidateAddr(servers);
	// ValidateListe(servers);
	// ValidateErrorPage(servers);
	// ValidateClientMaxBodySize(servers);
	// ValidateServerName(servers);
	// ValidateLocations(servers);
}