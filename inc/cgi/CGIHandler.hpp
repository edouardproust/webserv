#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "typedefs.hpp"
#include <string>

class CGIHandler {

	// Not used
	CGIHandler();
	CGIHandler(CGIHandler const&);
	~CGIHandler();
	CGIHandler&	operator=(CGIHandler const&);

	public:

		static void CGIHandler::handleRequest(const std::string& scriptPath,
			const std::string& executor,
			const std::string& method,
			const Headers& headers,
			const std::string& queryString,
			const std::string& body,
			const std::string& contentType);

};

#endif