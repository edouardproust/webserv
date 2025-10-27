#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "http/Request.hpp"
#include "http/Response.hpp"
#include "config/LocationBlock.hpp"
#include "utils/utils.hpp"
#include "typedefs.hpp"

class CGIHandler {

	std::string					_filePath;
	std::string					_extension;
	std::string					_executor;
	std::vector<std::string>	_envp;
	std::vector<std::string>	_argv;
	std::string					_cgiOutput;

	// Not used
	CGIHandler(CGIHandler const&);
	CGIHandler&	operator=(CGIHandler const&);

	void	_buildEnvp(Request const&, std::string const&, std::string const&);
	void	_buildArgv(std::string const&, std::string const&);

	static std::string			_headerToEnvVar(std::string const&);

	static std::vector<char*>	_toCharPtrArray(const std::vector<std::string>&);


	public:

		CGIHandler();
		~CGIHandler();

		Response handleRequest(Request const&, LocationBlock const*, std::string const&);


		std::string const&				getFilePath() const;
		std::string const&				getExecutor() const;
		std::vector<std::string> const&	getEnvp() const;
		std::vector<std::string> const&	getArgv() const;
		std::string const&				getCgiOutput() const;

};

std::ostream&	operator<<(std::ostream&, CGIHandler const&);

#endif