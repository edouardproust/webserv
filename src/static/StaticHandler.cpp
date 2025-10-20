#include "static/StaticHandler.hpp"
#include "utils/utils.hpp"

/**
 * Very likely needed information (to be confirmed):
 * - _request.getMethod(),
 * - _request.getHeaders()
 */
Response	StaticHandler::handleRequest(std::string const& filePath, Request const& req) {
	(void)req; // to silence unused parameter warnings
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

Response	StaticHandler::handleError(ParseStatus status) {
	// This is a placeholder implementation
	std::string errorPage = "<html><body><h1>Error " + utils::toString(status) + "</h1></body></html>";

	// Build dummy response to allow compilation
	Response response;
	response.setError(status);
	return response;
}
