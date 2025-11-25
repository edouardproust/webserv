#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "cgi/CgiParams.hpp"
#include "http/Request.hpp"
#include "http/Response.hpp"
#include "config/LocationBlock.hpp"
#include "config/HostPortPair.hpp"
#include "static/StaticHandler.hpp"
#include "utils/signal.hpp"
#include "utils/utils.hpp"
#include "utils/typedefs.hpp"
#include "utils/Log.hpp"
#include "fcntl.h"
#include <sys/wait.h>

/**
 * Handles execution of CGI scripts.
 *
 * Resource-type RAII class: manages pipes and child processes, non-copyable.
 */
class CgiHandler
{
	static size_t const	_TIMEOUT_MS;
	static size_t const	_STEP_MS;
	static size_t const	_READ_BUFFER;

	Request const&		_request;
	CgiParams const&	_cgiParams;

	int			_stdinPipe[2];
	int			_stdoutPipe[2];
	int			_stderrPipe[2];
	std::string	_tmpFile;

	std::string	_cgiOutput;
	std::string	_cgiError;

	pid_t		_forkAndExec() const;
	void		_prepareIo();
	Response	_handleStatus(int);
	void		_redirectIoInChild() const;
	void		_setNonBlocking(int);
	bool		_waitWithTimeout(pid_t, int&, size_t);
	void		_readPipes();
	void		_cleanupPipes();

	// Defaut constructore and copy / assignation are forbidden
	CgiHandler();
	CgiHandler(CgiHandler const&);
	CgiHandler&	operator=(CgiHandler const&);

	public:

		CgiHandler(Request const&, CgiParams const&);
		~CgiHandler();

		Response execute();

		CgiParams const&	getCgiParams() const;
		std::string const&	getCgiOutput() const;
		std::string const&	getCgiError() const;

		class ExecException: public std::runtime_error {
			public:
				ExecException(std::string const&);
		};

		class TimeoutException: public std::runtime_error {
			public:
				TimeoutException(std::string const&);
		};
};

std::ostream&	operator<<(std::ostream&, CgiHandler const&);

#endif