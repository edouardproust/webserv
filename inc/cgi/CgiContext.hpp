#ifndef CGI_CONTEXT_HPP
#define CGI_CONTEXT_HPP

#include "http/Request.hpp"

/**
 * Holds runtime state for a single CGI execution (PID, pipes, timestamps).
 *
 * Resource-like entity: uniquely represents a running CGI process and its
 * associated file descriptors. Copying would duplicate ownership of
 * process-related resources, which is unsafe. Therefore the class is
 * not default-constructible, not copyable, not assignable.
 *
 * The class stores a non-owning pointer to the originating Request.
 */
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

	// Not default-constructible, not copyable, not assignable
	CgiContext();
	CgiContext(CgiContext const&);
	CgiContext& operator=(CgiContext const&);

	public:

		CgiContext(pid_t, int, int, int, bool, Request const*);
		~CgiContext();

		pid_t	getPid() const;
		int		getClientFd() const;
		int		getStdoutPipe() const;
		int		getStderrPipe() const;
};

#endif