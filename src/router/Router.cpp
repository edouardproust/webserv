#include "router/Router.hpp"
#include "router/RoutingDecision.hpp"
#include "static/StaticHandler.hpp"
#include "cgi/CGIHandler.hpp"
#include "utils/utils.hpp"
#include "constants.hpp"
#include <iostream>

Router::Router() {}

Router::~Router() {}

/**
 * Dispatch the request to corresponding handler by crossing data between Config and Request.
 *
 * Notes:
 * - The Router does not check the validity of the data inside Config and Request objects,
 *   so the data needs to be 100% correct when passed to the Router. Related parsers are
 *   responsible for checking the data thoroughly.
 */
Response Router::dispatchRequest(Config const& config, Request const& request, HostPortPair const& listen) {
	RoutingDecision decision(config, request, listen);
	if (DEVMODE) std::cout << decision << std::endl;
	return Response();
	/*
    ParseStatus requestStatus = _request.getStatus();
    if (requestStatus != PARSE_SUCCESS) {
		_sendResponse(StaticHandler::handleError(requestStatus)); // TODO: add checks in Reponse ?
        return;
    }
    ServerBlock const& server = _findMatchingServer();
    _setMatchingLocation(server.getLocations(), _request.getPath());
	if (!_location)
		_location = &server.getDefaultLocation();
	if (DEVMODE)
		std::cout << *this << std::endl;
    std::string executor = "";
	std::string const& extension = utils::getFileExtension(_request.getPath());
    if (_location->isCgiLocation() && !extension.empty()) { // the request path matches a CGI location and has an extension
        executor = _location->getCgiExecutor(extension); // if no executor found for this extension: executor = ""
    }
	if (!executor.empty()) { // the file extension is handled by the CGI config of this location
        std::string scriptPath = _resolveScriptPath(_request.getPath(), _location);
		_sendResponse(CGIHandler::handleRequest(scriptPath, executor, _request));
    } else { // not a CGI request or file extension not handled by this location
        std::string filePath = _resolveFilePath(_request.getPath(), _location);
		_sendResponse(StaticHandler::handleRequest(filePath, _request));
    }
	*/
}

/**
 * //TODO: Maybe this will be moved into network listening loop
 */
void	Router::sendResponse(Response const& response) {
	// TODO
	(void)response;
}

std::string	Router::_resolveScriptPath(std::string const& requestPath, LocationBlock const* location) const {
	// TODO
	return "/var/www/cgi-bin" + requestPath.substr(location->getPath().length());
	(void)requestPath; (void)location; // to silence unused parameter warning
}

std::string	Router::_resolveFilePath(std::string const& requestPath, LocationBlock const* location) const {
	// TODO
	return location->getRoot() + requestPath;
	(void)requestPath; (void)location; // to silence unused parameter warning
}

