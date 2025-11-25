#ifndef CGI_CONTEXT_HPP
#define CGI_CONTEXT_HPP

#include "http/Request.hpp"

class CgiContext
{
	pid_t			pid;
	int				clientFd;
	int				stdoutPipe;
	int				stderrPipe;
	std::string		output;
	std::string		error;
	time_t			startTime;
	bool			connectionClose;  // Store connection flag
	Request const*	originalRequest; // Reference to the origin Request (non-owning)

	CgiContext(pid_t, int, int, int, bool, Request const*);
};

#endif