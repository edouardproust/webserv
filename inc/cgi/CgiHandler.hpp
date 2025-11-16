#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "http/Request.hpp"
#include "http/Response.hpp"
#include "config/LocationBlock.hpp"
#include "utils/utils.hpp"
#include "typedefs.hpp"
#include "utils/Log.hpp"
#include "fcntl.h"
#include <sys/wait.h>

/**
 * Entity type class
 */
class CgiHandler {

	std::string					_scriptName;
	std::string					_extension;
	std::string					_executor;
	std::vector<std::string>	_envp;
	std::vector<std::string>	_argv;
	std::string					_cgiOutput;
	std::string					_cgiError;

	int							_stdinPipe[2];
	int							_stdoutPipe[2];
	int							_stderrPipe[2];

	void		_buildEnvp(Request const&, std::string const&);
	void		_buildArgv();
	pid_t		_forkAndExec() const;
	void		_prepareIo(std::string const&, std::string const&);
	Response	_handleStatus(int) const;
	void		_redirectIoInChild() const;
	void		_setNonBlocking(int fd);
	bool		_waitWithTimeout(pid_t pid, int& status, size_t timeout_ms);
	void		_readPipes();
	void		_cleanupPipes();

	// static utils
	static std::string			_headerToEnvVar(std::string const&);
	static std::vector<char*>	_toCharPtrArray(const std::vector<std::string>&);

	// Copy disabled - would duplicate pipes and cause double-close
	CgiHandler(CgiHandler const&);
	CgiHandler&	operator=(CgiHandler const&);

	public:

		CgiHandler();
		~CgiHandler();

		Response run(Request const&, LocationBlock const*, std::string const&);

		std::string const&				getScriptName() const;
		std::string const&				getExecutor() const;
		std::vector<std::string> const&	getEnvp() const;
		std::vector<std::string> const&	getArgv() const;
		std::string const&				getCgiOutput() const;
		std::string const&				getCgiError() const;

		class ExecException: public std::runtime_error {
			public:
				ExecException(std::string const& msg);
		};

		class TimeoutException: public std::runtime_error {
			public:
				TimeoutException(std::string const& msg);
		};

};

std::ostream&	operator<<(std::ostream&, CgiHandler const&);

#endif