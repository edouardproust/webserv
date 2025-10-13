#include "cgi/CGIHandler.hpp"
#include "utils/utils.hpp"

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
	for (const auto& header : headers) {
		std::string envVar = "HTTP_" + _headerToEnvVar(header.first);
		setenv(envVar.c_str(), header.second.c_str(), 1);
	}

	// 4. Gestion of the request body for POST/PUT methods
	if (method == "POST" || method == "PUT") {
		_writeBodyToStdin(body);  // Écrit dans le pipe stdin du processus CGI
	}

	// 5. Execute script
	_executeScript(executor, scriptPath);
}