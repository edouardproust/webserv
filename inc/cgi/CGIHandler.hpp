#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "http/Request.hpp"
#include "http/Response.hpp"
#include "typedefs.hpp"
#include <string>

class CGIHandler {

	// Not used
	CGIHandler();
	CGIHandler(CGIHandler const&);
	~CGIHandler();
	CGIHandler&	operator=(CGIHandler const&);

	static std::string	_headerToEnvVar(const std::string&);
	static void	_writeBodyToStdin(const std::string&);
	static void	_executeScript(const std::string&, const std::string&);

	public:

		static Response handleRequest(const std::string&, const std::string&, Request);

};

#endif