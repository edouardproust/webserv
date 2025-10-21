#include "static/StaticHandler.hpp"
#include "utils/utils.hpp"
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <cctype>

/*StaticHandler::StaticHandler() : _mimeTypes(_initMimeTypes()) {}

StaticHandler::StaticHandler(const StaticHandler& other) : _mimeTypes(other._mimeTypes) {}

StaticHandler& StaticHandler::operator=(const StaticHandler& other)
{
	if (this != other)
		_mimeTypes = other._mimeTypes;
	return *this;
}

StaticHander::~StaticHander() {}

std::map<std::string, std::string> StaticHandler::_initMimeTypes() const
{
	std::map<std::string, std::string> types;
	types["html"] = "text/html";
	types["css"] = "text/css";
	types["txt"] = "text/plain";
	types["jpg"] = "image/jpeg";
	types["jpeg"] = "image/jpeg";
	types["png"] = "image/png";
	types["gif"] = "image/gif";
	types["ico"] = "image/x-icon";
	types["js"] = "application/javascript";
	types["json"] = "application/json";
	types["pdf"] = "application/pdf";
	return types;
}*/

/**
 * Very likely needed information (to be confirmed):
 * - _request.getMethod(),
 * - _request.getHeaders()
 */
Response	StaticHandler::handleRequest(std::string const& filePath, Request const& request) {
	(void)request; // to silence unused parameter warnings
	// This is a placeholder implementation
	std::string staticContent = "<html><body><h1>Static Content from " + filePath + "</h1></body></html>";

	// Build dummy response to allow compilation
	Response response;
	std::string content = "[This is the content of the static file]";
	response.setStatusCode(200);
	response.setHeader("Content-Type", "text/html");
	response.setHeader("Content-Length", utils::toString(content.size()));
	response.setHeader("Date", response.getCurrentDate());
	response.setBody(content);
	return response;
}

Response	StaticHandler::handleError(HttpStatus status) {
	// This is a placeholder implementation
	std::string errorPage = "<html><body><h1>Error " + utils::toString(status.getCode()) + "</h1></body></html>";

	// Build dummy response to allow compilation
	Response response;
	response.setError(status.getCode());
	return response;
}

/*std::string StaticHandler::_generateErrorPage(int statusCode, const std::string& reasonPhrase) const
{
	std::stringstream html;
	std::string reasonPhrase = _getReasonPhrase(_statusCode);

	html << "<html>\n"
		 << "  <head>\n"
		 << "    <title>" << _statusCode << " " << reasonPhrase << "</title>\n"
		 << "  </head>\n"
		 << "  <body>\n"
		 << "    <center><h1>" << _statusCode << " " << reasonPhrase << "</h1></center>\n"
		 << "    <hr><center>webserv/1.0</center>\n"
		 << "  </body>\n"
		 << "</html>";
	return html.str();
}*/
