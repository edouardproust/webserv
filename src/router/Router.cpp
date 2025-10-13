#include "router/Router.hpp"
#include "static/StaticHandler.hpp"
#include "cgi/CGIHandler.hpp"
#include "utils/utils.hpp"
#include "constants.hpp"
#include <iostream>

Router::Router(Request const& request, std::vector<ServerBlock> const& servers)
: _request(request), _servers(servers) {}

Router::~Router() {}

void Router::dispatchRequest() const {
    ParseStatus requestStatus = _request.getStatus();
    if (requestStatus != PARSE_SUCCESS) {
        StaticHandler::sendErrorPageContent(requestStatus);
        return;
    }
    const ServerBlock& server = _findMatchingServer(_request.getHost(), _request.getPort());
    const LocationBlock& location = _findMatchingLocation(server, _request.getPath());
    std::string executor;
    if (location.isCgiLocation()) { // the request path matches a CGI location
        executor = location.getCgiExecutor(utils::getFileExtension(_request.getPath()));
    }
	if (!executor.empty()) { // the file extension is handled by the CGI config of this location
        std::string scriptPath = _resolveScriptPath(_request.getPath(), location);
        std::string executor = location.getCgiExecutor(utils::getFileExtension(_request.getPath()));
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
        std::string filePath = _resolveFilePath(_request.getPath(), location);
        StaticHandler::sendStaticContent(
            filePath,
            _request.getMethod(),
            _request.getHeaders(),
            location.getRoot(),
            location.getIndexFiles()
        );
    }
}



Request const&	Router::getRequest() const {
	return _request;
}

std::vector<ServerBlock> const&	Router::getServers() const {
	return _servers;
}

std::ostream&	operator<<(std::ostream& os, Router const& rhs) {
	Request const& request = rhs.getRequest();
	os << "Router:\n";
	os << "- Request method: " << request.getMethod() << "\n";
	os << "- Request path: " << request.getPath() << "\n";
	os << "- Request version: " << request.getVersion() << "\n";
	os << "- Headers: " << request.getHeaders().size() << std::endl;
	for (std::map<std::string, std::string>::const_iterator it = request.getHeaders().begin();
		it != request.getHeaders().end(); ++it) {
		os << "  - " << it->first << ": " << it->second << "\n";
	}
	return os;
}
