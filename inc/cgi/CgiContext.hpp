#ifndef CGI_CONTEXT_HPP
#define CGI_CONTEXT_HPP

#include "cgi/CgiData.hpp"
#include "cgi/SafePipe.hpp"

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
	pid_t				_pid;
	int					_clientFd;
	SafePipe&			_stdinPipe; // TODO remove ?
	SafePipe&			_stdoutPipe;
	SafePipe&			_stderrPipe;
	Request	const&		_request; // non-owning
	ErrorPages const&	_errorPages; // non-owning

	time_t				_startTime;
	std::string			_output; // owning
	std::string			_error; // owning

	// Not default-constructible, not copyable, not assignable
	CgiContext();
	CgiContext(CgiContext const&);
	CgiContext& operator=(CgiContext const&);

	public:

		CgiContext(pid_t, int, SafePipe&, SafePipe&, SafePipe&, CgiData const&); // TODO stdin to remove?
		~CgiContext();

		void	appendOutput(const char*, size_t);
    	void	appendError(const char*, size_t);
		void	setStartTime();

		pid_t				getPid() const;
		int					getClientFd() const;
		SafePipe&			getStdinPipe() const; // TODO remove?
		SafePipe&			getStdoutPipe() const;
		SafePipe&			getStderrPipe() const;
		Request const&		getRequest() const;
		ErrorPages const&	getErrorPages() const;
		time_t const&		getStartTime() const;
		std::string	const&	getOutput() const;
		std::string const&	getError() const;
};

#endif