#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "http/Request.hpp"
#include "http/Response.hpp"
#include "config/LocationBlock.hpp"
#include "utils/utils.hpp"
#include "typedefs.hpp"
#include <sys/wait.h>

class CgiHandler {

	std::string					_filePath;
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
	pid_t		_forkAndExec();
	void		_communicateWithChild(Request const&);
	Response	_handleStatus(int);
	void		_redirectIOInChild() const;

	// static utils
	static std::string			_headerToEnvVar(std::string const&);
	static std::vector<char*>	_toCharPtrArray(const std::vector<std::string>&);

	// Not used
	CgiHandler(CgiHandler const&);
	CgiHandler&	operator=(CgiHandler const&);

	public:

		CgiHandler();
		~CgiHandler();

		Response run(Request const&, LocationBlock const*, std::string const&);

		std::string const&				getFilePath() const;
		std::string const&				getExecutor() const;
		std::vector<std::string> const&	getEnvp() const;
		std::vector<std::string> const&	getArgv() const;
		std::string const&				getCgiOutput() const;
		std::string const&				getCgiError() const;

		class ExecException: public std::runtime_error {
			public:
				explicit ExecException(std::string const& msg);
		};

};

std::ostream&	operator<<(std::ostream&, CgiHandler const&);

#endif