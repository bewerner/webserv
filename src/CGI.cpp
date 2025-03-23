#include "webserv.hpp"

void	CGI::init_pipes(void)
{
	if (fail)
		return ;

	// if
	// (
	// 	pipe(pipe_into_cgi) < 0 ||
	// 	pipe(pipe_from_cgi) < 0 ||
	// 	fcntl(pipe_into_cgi[0], F_SETFL, O_NONBLOCK) < 0 ||
	// 	fcntl(pipe_into_cgi[1], F_SETFL, O_NONBLOCK) < 0 ||
	// 	fcntl(pipe_from_cgi[0], F_SETFL, O_NONBLOCK) < 0 ||
	// 	fcntl(pipe_from_cgi[1], F_SETFL, O_NONBLOCK) < 0
	// )
	// 	fail = true;


	if
	(
		pipe(pipe_into_cgi) < 0 ||
		pipe(pipe_from_cgi) < 0
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
		pipe_into_cgi[0] = -1;
		close(pipe_from_cgi[1]);
		pipe_from_cgi[1] = -1;
	}
	else // in child process
	{
		close(pipe_into_cgi[1]);
		pipe_into_cgi[1] = -1;
		close(pipe_from_cgi[0]);
		pipe_from_cgi[0] = -1;
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

CGI::~CGI(void)
{
	if (pipe_into_cgi[0] >= 0)
		close(pipe_into_cgi[0]);
	if (pipe_into_cgi[1] >= 0)
		close(pipe_into_cgi[1]);
	if (pipe_from_cgi[0] >= 0)
		close(pipe_from_cgi[0]);
	if (pipe_from_cgi[1] >= 0)
		close(pipe_from_cgi[1]);
}
