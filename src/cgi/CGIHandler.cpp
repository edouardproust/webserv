#include "cgi/CGIHandler.hpp"
#include "utils/utils.hpp"
#include <iostream> //DEBUG

void CGIHandler::handleRequest(const std::string& scriptPath,
	const std::string& executor,
	const std::string& method,
	const Headers& headers,
	const std::string& queryString,
	const std::string& body,
	const std::string& contentType)
{
	// Essential CGI env. variables
	setenv("REQUEST_METHOD", method.c_str(), 1);
	setenv("QUERY_STRING", queryString.c_str(), 1);
	setenv("CONTENT_TYPE", contentType.c_str(), 1);
	setenv("CONTENT_LENGTH", utils::toString(body.length()).c_str(), 1);
	setenv("SCRIPT_FILENAME", scriptPath.c_str(), 1);
	setenv("SCRIPT_NAME", scriptPath.c_str(), 1); // Souvent requis

	// Other important env. variables
	setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);
	setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
	setenv("SERVER_SOFTWARE", "webserv/42", 1);

	// Conversion of HTTP headers into env. variables (CGI standard)
	for (Headers::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        std::string envVar = "HTTP_" + _headerToEnvVar(it->first);
        setenv(envVar.c_str(), it->second.c_str(), 1);
    }

	// 4. Gestion of the request body for POST/PUT methods
	if (method == "POST" || method == "PUT") {
		_writeBodyToStdin(body);  // Écrit dans le pipe stdin du processus CGI
	}

	// 5. Execute script
	_executeScript(executor, scriptPath);
}

std::string	CGIHandler::_headerToEnvVar(const std::string& header) {
	std::string envVar;
	for (size_t i = 0; i < header.length(); ++i) {
		char c = header[i];
		if (c == '-')
			envVar += '_';
		else
			envVar += std::toupper(c);
	}
	return envVar;
}

void	CGIHandler::_writeBodyToStdin(const std::string& body) {
	std::cout << "[TODO: Writing body to CGI stdin]" << body << std::endl;
	(void)body; // to silence unused parameter warning
}

void	CGIHandler::_executeScript(const std::string& executor, const std::string& scriptPath) {
	std::cout << "[TODO: Executing CGI script]" << std::endl;
	std::cout << "Executor: " << executor << std::endl;
	std::cout << "Script Path: " << scriptPath << std::endl;
}
