#include "cgi/CGIHandler.hpp"
#include "utils/utils.hpp"
#include <iostream> //DEBUG

CGIHandler::CGIHandler() {}

Response	CGIHandler::handleRequest(const std::string& scriptPath, const std::string& executor, Request req)
{
	std::string const& method = req.getMethod();
	std::string const& body = req.getBody();
	// Essential CGI env. variables
	setenv("REQUEST_METHOD", method.c_str(), 1);
	setenv("QUERY_STRING", req.getQueryString().c_str(), 1);
	setenv("CONTENT_TYPE", req.getContentType().c_str(), 1);
	setenv("CONTENT_LENGTH", utils::toString(body.length()).c_str(), 1);
	setenv("SCRIPT_FILENAME", scriptPath.c_str(), 1);
	setenv("SCRIPT_NAME", scriptPath.c_str(), 1); // Souvent requis
	// Other important env. variables
	setenv("GATEWAY_INTERFACE", "CGI/1.1", 1); // TODO: make dynamic ?
	setenv("SERVER_PROTOCOL", "HTTP/1.1", 1); // TODO: make dynamic ?
	setenv("SERVER_SOFTWARE", "webserv/42", 1); // TODO: make dynamic ?
	// Conversion of HTTP headers into env. variables (CGI standard)
	Headers headers = req.getHeaders();
	for (Headers::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        std::string envVar = "PARSE_" + _headerToEnvVar(it->first);
        setenv(envVar.c_str(), it->second.c_str(), 1);
    }
	// 4. Gestion of the request body for POST/PUT methods
	if (method == "POST" || method == "PUT") {
		_writeBodyToStdin(body);  // Écrit dans le pipe stdin du processus CGI
	}
	// 5. Execute script
	_executeScript(executor, scriptPath);
	// Build and return Response
	return Response(); // Return an empty Response for now to allow compilation
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
