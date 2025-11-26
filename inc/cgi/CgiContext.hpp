#ifndef CGI_CONTEXT_HPP
#define CGI_CONTEXT_HPP

#include "http/Request.hpp"

// TODO Canonical class ? And class comment
class CgiContext
{
	pid_t			_pid;
	int				_clientFd;
	int				_stdoutPipe;
	int				_stderrPipe;
	std::string		_output;
	std::string		_error;
	time_t			_startTime;
	bool			_connectionClose;  // Store connection flag
	Request const*	_originalRequest; // Reference to the origin Request (non-owning)

	public:

		CgiContext(pid_t, int, int, int, bool, Request const*);

		pid_t	getPid() const;
		int		getClientFd() const;
		int		getStdoutPipe() const;
		int		getStderrPipe() const;
};

#endif