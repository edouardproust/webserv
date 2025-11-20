#ifndef PIPES_CGI_HANDLER_HPP
#define PIPES_CGI_HANDLER_HPP

#include "cgi/CgiHandler.hpp"
#include <sys/wait.h>

// TODO canonical form?
// TODO class desc
class PipeCgiHandler : public CgiHandler
{
	static size_t const	_TIMEOUT_MS;
	static size_t const	_STEP_MS;
	static size_t const	_READ_BUFFER;

	int	_stdinPipe[2];
	int	_stdoutPipe[2];
	int	_stderrPipe[2];

	void	_prepareIo();


	pid_t	_forkAndExec() const;
	bool	_waitWithTimeout(pid_t, int&, size_t);
	void	_writeBodyToPipe(std::string const&, std::string const&);
	void	_redirectIoInChild() const;
	void	_readPipes();

	public:

		PipeCgiHandler();
		~PipeCgiHandler();

		Response execute(Request const&, LocationBlock const*, std::string const&, HostPortPair const&);
};

#endif