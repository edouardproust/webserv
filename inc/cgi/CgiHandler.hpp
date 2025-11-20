#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "config/LocationBlock.hpp"
#include "config/HostPortPair.hpp"
#include "http/Response.hpp"
#include "fcntl.h"

// TODO canonical form?
// TODO class desc
/**
 * Abstract class
 */
class CgiHandler
{
	protected:

		std::string					_scriptName;
		std::string					_extension;
		std::string					_executor;
		std::string					_cgiOutput;
		std::string					_cgiError;
		std::vector<std::string>	_argv;
		std::vector<std::string>	_envp;

		CgiHandler();

		void			_buildEnvp(Request const&, std::string const&, HostPortPair const&);
		void			_buildArgv();
		Response		_handleStatus(int);

		static void					_setNonBlocking(int&);
		static void					_closeFd(int&);
		static void					_initPipe(int[2]);
		static void					_closePipe(int[2]);
		static std::string			_headerToEnvVar(std::string const&);
		static std::vector<char*>	_toCharPtrArrayInChild(const std::vector<std::string>&);

	public:

		static size_t const	_FILES_THRESHOLD;

		virtual ~CgiHandler();
		virtual Response execute(Request const&, LocationBlock const*, std::string const&, HostPortPair const&) = 0;

		std::string const&				getScriptName() const;
		std::string const&				getExecutor() const;
		std::string const&				getCgiOutput() const;
		std::string const&				getCgiError() const;
		std::vector<std::string> const&	getArgv() const;
		std::vector<std::string> const&	getEnvp() const;

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