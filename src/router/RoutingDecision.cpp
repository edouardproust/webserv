#include "router/RoutingDecision.hpp"

RoutingDecision::RoutingDecision(Config const& c, Request const& r, HostPortPair const& l)
: _config(c), _request(r), _listen(l), _server(NULL), _location(NULL),
  _decision(ERROR), _errorSlug("internal_server_error") {
	_setServer();
	_setLocation();
	_setFilePath();
	_makeDecision();
}

RoutingDecision::~RoutingDecision() {}

/**
 * Notes:
 * - HTTP version is checked during request parsing.
 */
void	RoutingDecision::_makeDecision() {
	if (!_location || !_server)
		_setError("internal_server_error");
	else if (_filePath.empty())
		_setError("forbidden"); // transversal attack
	else if (!_location->isAllowedMethod(_request.getMethod()))
		_setError("method_not_allowed");
	else if (!_location->isAllowedClientBodySize(_request.getBody().length(), _request.getMethod()))
		_setError("content_too_large");
	else if (_location->isRedirection())
		_decision = REDIRECTION;
	else if (_location->isCgi(utils::getFileExtension(_request.getPath())))
		_decision = CGI;
	else
		_decision = STATIC;
}

void	RoutingDecision::_setServer() {
	_server = NULL;
	std::vector<ServerBlock> const& servers = _config.getServers();
	ServerBlock const* wildcardMatch = NULL;
	for (size_t i = 0; i < servers.size(); ++i) {
		std::set<HostPortPair> const& serverListens = servers[i].getListen();
		for (std::set<HostPortPair>::const_iterator it = serverListens.begin(); it != serverListens.end(); ++it) {
			if (*it == _listen) {
				_server = &servers[i]; // exact match (priority 1)
				return;
			}
			if (!wildcardMatch && it->isWildcardFor(_listen))
                wildcardMatch = &servers[i];
		}
	}
	if (wildcardMatch) _server = wildcardMatch; // wildcard match (priority 2)
	else _server = &servers[0]; // fallback for security (priority 3), but should never happen
}

void	RoutingDecision::_setLocation() {
	if (!_server)
		_setServer();
	std::vector<LocationBlock> const& locations = _server->getLocations();
	LocationBlock const* best = NULL;
	size_t longest = 0;
	std::string const& reqPath = _request.getPath();
	for (size_t i = 0; i < locations.size(); ++i) {
		const std::string& locPath = locations[i].getPath();
		if (reqPath.compare(0, locPath.size(), locPath) == 0) { // prefix match
			if (locPath == "/" || reqPath.size() == locPath.size() || reqPath[locPath.size()] == '/') {
				if (locPath.size() > longest) {
					longest = locPath.size();
					best = &locations[i];
				}
			}
		}
	}
	if (!best)
		best = &LocationBlock::getDefaultLocation(_server);
	_location = best;
}

/**
 * May set _filePath to an empty string in case of root path traversal attempt.
 */
void	RoutingDecision::_setFilePath() {
	if (!_server)
		_setServer();
	if (!_location)
		_setLocation();
	std::string joinedPath = utils::securedPathsJoin(_location->getRoot(), _request.getPath());
	_filePath = joinedPath;
}

void	RoutingDecision::_setError(std::string const& errorSlug) {
	_decision = ERROR;
	_errorSlug = errorSlug;
}

RoutingDecision::Decision const&	RoutingDecision::getDecision() const {
	return _decision;
}

Request const&	RoutingDecision::getRequest() const {
	return _request;
}

ServerBlock const*	RoutingDecision::getServer() const {
	return _server;
}

LocationBlock const*	RoutingDecision::getLocation() const {
	return _location;
}

std::string const&	RoutingDecision::getFilePath() const {
	return _filePath;
}

std::string const&	RoutingDecision::getErrorSlug() const {
	return _errorSlug;
}

std::ostream&	operator<<(std::ostream& os, RoutingDecision const& rhs) {
	int d = rhs.getDecision();
	os << "RoutingDecision:\n"
		<< "- Decision: " << (d == RoutingDecision::ERROR ? "ERROR"
			: (d == RoutingDecision::REDIRECTION ? "REDIRECTION"
				: (d == RoutingDecision::STATIC ? "STATIC"
					: d == RoutingDecision::CGI ? "CGI"
						: "UNKNOWN"))) << "\n"
		<< "- File path: '" << rhs.getFilePath() << "'\n"
		<< "- Matching server:\n"
		<< "  - root: '" << rhs.getServer()->getRoot() << "'\n"
		<< "- Request path: '" << rhs.getRequest().getPath() << "'\n"
		<< "- Matching location:";
	LocationBlock const* location = rhs.getLocation();
	if (location) os << "\n" << *location;
	else os << "[empty]";

	return os;
}
