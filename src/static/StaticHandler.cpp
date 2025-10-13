#include "static/StaticHandler.hpp"
#include "utils/utils.hpp"

void StaticHandler::sendErrorPageContent(ParseStatus status) {
	// Implementation to send an error page based on the status
	// This is a placeholder implementation
	std::string errorPage = "<html><body><h1>Error " + utils::toString(status) + "</h1></body></html>";
	// Send the errorPage content to the client via "network" module
}

void StaticHandler::sendStaticContent(
	std::string const& filePath,
	std::string const& method,
	std::map<std::string, std::string> const& headers,
	std::string const& root,
	std::vector<std::string> const& indexFiles
) {
	(void)method; (void)headers; (void)root; (void)indexFiles; // to silence unused parameter warnings
	// Implementation to send static content based on the filePath and other parameters
	// This is a placeholder implementation
	std::string staticContent = "<html><body><h1>Static Content from " + filePath + "</h1></body></html>";
	// Send the staticContent to the client via "network" module
}