#include "webserv.hpp"

void	CGI::init_pipes(void)
{
	if (fail)
		return ;

	if
	(
		pipe(pipe_into_cgi) < 0 ||
		pipe(pipe_from_cgi) < 0 ||
		fcntl(pipe_into_cgi[1], F_SETFL, O_NONBLOCK) < 0 ||
		fcntl(pipe_from_cgi[0], F_SETFL, O_NONBLOCK) < 0
	)
		fail = true;
}

void	CGI::fork(void)
{
	if (fail)
		return ;

	pid = ::fork();
	if (pid < 0)
		fail = true;
}

void	CGI::setup_io(void)
{
	if (fail)
		return ;

	if (pid != 0) // in parent process
	{
		close(pipe_into_cgi[0]);
		close(pipe_from_cgi[1]);
	}
	else // in child process
	{
		close(pipe_into_cgi[1]);
		close(pipe_from_cgi[0]);
		if (dup2(pipe_into_cgi[0], STDIN_FILENO) < 0)
			fail = true;
		if (fail)
			return ;
		close(pipe_into_cgi[0]);
		if (dup2(pipe_from_cgi[1], STDOUT_FILENO) < 0)
			fail = true;
		if (fail)
			return ;
		close(pipe_from_cgi[1]);
	}
}

void	CGI::exec(const Server& server, const Request& request, const Response& response, const Connection& connection)
{
	if (fail || pid != 0)
		return ;

		
	std::string target = response.response_target;
	normalize_path(target);
	collapse_absolute_path(target);
	std::string relative_target = target;
	if(target.rfind('/') != 0)
		relative_target = target.substr(std::filesystem::current_path().string().length());
	std::string pwd = target.substr(0, target.rfind('/'));
	if (pwd.empty())
		pwd = "/";

	std::string remote_addr, server_addr;
	uint8_t* ip = (uint8_t*)&server.sockaddr.sin_addr;
	if (ip)
		remote_addr = (std::ostringstream{} << (int)ip[0] << '.' << (int)ip[1] << '.' << (int)ip[2] << '.' << (int)ip[3]).str();
	sockaddr_in serveraddr;
	socklen_t serveraddr_len = sizeof(serveraddr);
	getsockname(server.socket, (struct sockaddr*)&serveraddr, &serveraddr_len);
	ip = (uint8_t*)&serveraddr.sin_addr;
	if (ip)
		server_addr = (std::ostringstream{} << (int)ip[0] << '.' << (int)ip[1] << '.' << (int)ip[2] << '.' << (int)ip[3]).str();
	chdir(pwd.c_str());


	std::string env_path = std::getenv("PATH") ? std::getenv("PATH") : "/bin:/usr/bin:/usr/ucb:/usr/bsd:/usr/local/bin";
	std::string perl5lib = std::getenv("PERL5LIB") ? std::getenv("PERL5LIB") : "";

	std::vector<std::string> env_str;
	env_str.emplace_back("SERVER_SIGNATURE=");
	env_str.emplace_back("SERVER_PORT=" + std::to_string(server.port));
	env_str.emplace_back("DOCUMENT_ROOT=" + response.server_config->root);
	env_str.emplace_back("SCRIPT_FILENAME=" + target);
	env_str.emplace_back("REQUEST_URI=" + request.URI);
	env_str.emplace_back("SCRIPT_NAME=" + relative_target);
	env_str.emplace_back("REMOTE_PORT=" + std::to_string(ntohs(connection.sockaddr.sin_port)));
	env_str.emplace_back("PATH=" + env_path);
	env_str.emplace_back("PERL5LIB=" + perl5lib);
	env_str.emplace_back("CONTEXT_PREFIX=" + response.location_config->path);
	env_str.emplace_back("PWD=" + pwd);
	env_str.emplace_back("SERVER_ADMIN=[no address given]");
	env_str.emplace_back("REQUEST_SCHEME=http");
	env_str.emplace_back("PATH_TRANSLATED=" + target + response.path_info);
	env_str.emplace_back("REMOTE_ADDR=" + remote_addr);
	env_str.emplace_back("SERVER_NAME=" + request.host);
	env_str.emplace_back("HTTP_CACHE_CONTROL=no-cache");
	env_str.emplace_back("HTTP_HOST=" + request.host + ':' + std::to_string(server.port));
	env_str.emplace_back("HTTP_USER_AGENT=" + request.user_agent);
	env_str.emplace_back("HTTP_REFERER=" + request.referer);
	env_str.emplace_back("HTTP_ORIGIN=" + request.origin);
	env_str.emplace_back("HTTP_PRAGMA=no-cache");
	env_str.emplace_back("HTTP_SEC_FETCH_DEST=document");
	env_str.emplace_back("HTTP_SEC_FETCH_USER=?1");
	env_str.emplace_back("HTTP_CONNECTION=" + request.connection);
	env_str.emplace_back("SERVER_SOFTWARE=" + response.server);
	env_str.emplace_back("QUERY_STRING=");
	env_str.emplace_back("SERVER_ADDR=" + server_addr);
	env_str.emplace_back("GATEWAY_INTERFACE=CGI/1.1");
	env_str.emplace_back("SERVER_PROTOCOL=HTTP/1.1");
	env_str.emplace_back("REQUEST_METHOD=" + request.method);
	env_str.emplace_back("CONTEXT_DOCUMENT_ROOT=" + response.location_config->root + response.location_config->path);
	if (!response.path_info.empty())
		env_str.emplace_back("PATH_INFO=" + response.path_info);
	if (!request.transfer_encoding.empty())
		env_str.emplace_back("HTTP_TRANSFER_ENCODING=" + request.transfer_encoding);
	else
		env_str.emplace_back("CONTENT_LENGTH=" + std::to_string(request.content_length));
	if (!request.content_type.empty())
		env_str.emplace_back("CONTENT_TYPE=" + request.content_type);
	std::vector<char*> env;
	for (std::string& str : env_str)
		env.emplace_back(str.data());
	env.emplace_back(nullptr);

	char* argv[] = {target.data(), nullptr};

	if (execve(target.c_str(), argv, env.data()) == -1)
	{
		std::cerr << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" << std::endl;
		std::cerr << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" << std::endl;
		std::cerr << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" << std::endl;
		std::cerr << "          execve failed: " << target.c_str() << std::endl;
		std::cerr << "               " << strerror(errno) << std::endl;
		std::cerr << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" << std::endl;
		std::cerr << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" << std::endl;
		std::cerr << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" << std::endl;
		exit(EXIT_FAILURE);
	}
}

void	CGI::done_writing_into_cgi(void)
{
	close(pipe_into_cgi[1]);
}

void	CGI::done_reading_from_cgi(void)
{
	close(pipe_from_cgi[0]);
	eof = true;
}

bool	CGI::pollin(void) const
{
	return (!fail && revents_read_from_cgi != nullptr && (*revents_read_from_cgi & POLLIN || *revents_read_from_cgi & POLLHUP)); // include pollhup because unlike macOS, on linux pollin will not be triggered upon EOF
}

bool	CGI::pollout(void) const
{
	return (!fail && revents_write_into_cgi != nullptr && *revents_write_into_cgi & POLLOUT);
}

bool	CGI::is_running(void) const
{
	if (pid >= 0 && waitpid(pid, NULL, WNOHANG) != pid)
		return (true);
	return (false);
}

void	CGI::close(int& fd)
{
	if (fd >=0)
		::close(fd);
	fd = -1;
}

CGI::~CGI(void)
{
	close(pipe_into_cgi[0]);
	close(pipe_into_cgi[1]);
	close(pipe_from_cgi[0]);
	close(pipe_from_cgi[1]);
	if (pid >= 0 && waitpid(pid, NULL, WNOHANG) != pid)
		kill(pid, SIGKILL);
}
