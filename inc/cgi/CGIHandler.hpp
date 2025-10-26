#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "http/Request.hpp"
#include "http/Response.hpp"
#include "utils/utils.hpp"
#include "typedefs.hpp"

class CGIHandler {

	// Not used
	CGIHandler();
	CGIHandler(CGIHandler const&);
	~CGIHandler();
	CGIHandler&	operator=(CGIHandler const&);

	static std::vector<char*>	_buildEnvp(Request const&, std::string const&);
	static std::vector<char*>	_buildArgv(std::string const& executor, std::string const& scriptPath);
	static std::string			_headerToEnvVar(std::string const&);
	static void					_writeBodyToStdin(std::string const&);
	static void					_executeScript(std::string const&, std::string const&);

	public:

		static Response handleRequest(Request const&, std::string const&, std::string const&);

};

#endif