#include "router/Router.hpp"
#include "static/StaticHandler.hpp"
#include "cgi/CGIHandler.hpp"
#include "utils/utils.hpp"
#include "constants.hpp"
#include <iostream>

Router::Router(Request const& request, std::vector<ServerBlock> const& servers, HostPortPair const& listeningOn)
: _request(request), _servers(servers), _listeningOn(listeningOn), _location(NULL) {}

Router::~Router() {}

/**
 * Dispatch the request to corresponding handler by crossing data between Config and Request.
 *
 * Notes:
 * - The Router does not check the validity of the data inside Config and Request objects,
 *   so the data needs to be 100% correct when passed to the Router. Related parsers are
 *   responsible for checking the data thoroughly.
 */
void Router::dispatchRequest() {
    ParseStatus requestStatus = _request.getStatus();
    if (requestStatus != PARSE_SUCCESS) {
        StaticHandler::sendErrorPageContent(requestStatus);
        return;
    }
    ServerBlock const& server = _findMatchingServer();
    _setMatchingLocation(server.getLocations(), _request.getPath());
	if (!_location)
		_location = &server.getDefaultLocation();

    std::string executor = "";
	std::string const& extension = utils::getFileExtension(_request.getPath());
    if (_location->isCgiLocation() && !extension.empty()) { // the request path matches a CGI location and has an extension
        executor = _location->getCgiExecutor(extension); // if no executor found for this extension: executor = ""
    }
	if (!executor.empty()) { // the file extension is handled by the CGI config of this location
        std::string scriptPath = _resolveScriptPath(_request.getPath(), _location);
        std::string executor = _location->getCgiExecutor(extension);
        CGIHandler::handleRequest(
            scriptPath,
            executor,
            _request.getMethod(),
            _request.getHeaders(),
            _request.getQueryString(),
            _request.getBody(),
            _request.getContentType()
        );
    } else { // not a CGI request or file extension not handled by this location
        std::string filePath = _resolveFilePath(_request.getPath(), _location);
        StaticHandler::sendStaticContent(
            filePath,
            _request.getMethod(),
            _request.getHeaders()
        );
    }
}

ServerBlock const&	Router::_findMatchingServer() const {
	for (size_t i = 0; i < _servers.size(); ++i) {
		std::set<HostPortPair> serverListens = _servers[i].getListen();
		for (std::set<HostPortPair>::const_iterator it = serverListens.begin(); it != serverListens.end(); ++it) {
			if (*it == _listeningOn)
				return _servers[i];
		}
	}
	// fallback for security, but should never happen
	return _servers[0];
}

void	Router::_setMatchingLocation(std::vector<LocationBlock> const& locations, std::string const& path) {
	LocationBlock const* best = NULL;
	size_t longest = 0;
	for (size_t i = 0; i < locations.size(); ++i) {
		const std::string& locPath = locations[i].getPath();
		if (path.compare(0, locPath.size(), locPath) == 0) { // prefix match
			if (path.size() == locPath.size() || path[locPath.size()] == '/') { // prevents against false positives
				if (locPath.size() > longest) {
					longest = locPath.size();
					best = &locations[i];
				}
			}
		}
	}
	_location = best;
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

Request const&	Router::getRequest() const {
	return _request;
}

std::vector<ServerBlock> const&	Router::getServers() const {
	return _servers;
}

HostPortPair const&	Router::getListeningOn() const {
	return _listeningOn;
}

LocationBlock const* Router::getMatchingLocation() const {
	return _location;
}

std::ostream&	operator<<(std::ostream& os, Router const& rhs) {
	Request const& request = rhs.getRequest();
	os << "Router:\n";
	os << "- Listening on: " << rhs.getListeningOn().getHost() << ":" << rhs.getListeningOn().getPort() << "\n";
	os << "- Request method: " << request.getMethod() << "\n";
	os << "- Request path: " << request.getPath() << "\n";
	os << "- Request version: " << request.getVersion() << "\n";
	os << "- Headers: " << request.getHeaders().size() << std::endl;
	for (std::map<std::string, std::string>::const_iterator it = request.getHeaders().begin();
		it != request.getHeaders().end(); ++it) {
		os << "  - " << it->first << ": '" << it->second << "'\n";
	}
	LocationBlock const* matchingLoc = rhs.getMatchingLocation();
	os << "- Matching location:";
	if (matchingLoc) os << "\n" << *matchingLoc;
	else os << " [empty]";
	return os;
}
