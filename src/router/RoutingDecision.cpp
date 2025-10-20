#include "router/RoutingDecision.hpp"

RoutingDecision::RoutingDecision(Config const& c, Request const& r, HostPortPair const& l)
: _config(c), _request(r), _listen(l), _status(HTTP_BAD_REQUEST), _server(NULL),
  _location(NULL) {
	_setLocation();
  }

RoutingDecision::~RoutingDecision() {}

void	RoutingDecision::setStatus() {
	//TODO
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
			if (reqPath.size() == locPath.size() || reqPath[locPath.size()] == '/') { // prevents against false positives
				if (locPath.size() > longest) {
					longest = locPath.size();
					best = &locations[i];
				}
			}
		}
	}
	_location = best ? best : &LocationBlock::getDefaultLocation(NULL);
	std::cout << "DEBUG: " << *_location << std::endl;
}

void	RoutingDecision::setFinalPath() {
	// TODO
}

void	RoutingDecision::setCgiExecutor() {
	// TODO
}

void	RoutingDecision::setRedirectTarget() {
	// TODO
}

HttpStatus	RoutingDecision::getStatus() const {
	return _status;
}

ServerBlock const*	RoutingDecision::getServer() const {
	return _server;
}

LocationBlock const*	RoutingDecision::getLocation() const {
	return _location;
}

void	RoutingDecision::_resolveFinalPath() {
	if (!_location || !_server) {
		_status = HTTP_INTERNAL_SERVER_ERROR;
		return;
	}
	std::string path = _request.getPath();
	// Redirection (return) a priorité
	std::pair<int, std::string> ret = _location->getReturn();
	if (_location->isRedirection()) {
		_status = ret.first;
		_redirectTarget = ret.second;
		return;
	}
	// Vérifier si le chemin correspond à un dossier
	if (_location->isDirectory(path)) {
		// Chercher un fichier index
		std::string indexFile = _location->findIndexFile(path);
		if (!indexFile.empty()) {
			_finalPath = indexFile;
			_status = 200;
			return;
		}
		// Si autoindex est activé, servir la liste des fichiers
		if (_location->getAutoindex() == "on") {
			_finalPath = path;
			_status = 200; // La page autoindex sera générée dynamiquement
			return;
		}
		// Sinon, pas d'index et autoindex off => Forbidden
		_status = 403;
		return;
	}
	// Chemin normal (fichier) : vérifier existence si nécessaire
	_finalPath = path;
	_status = 200;
}


std::ostream&	operator<<(std::ostream& os, RoutingDecision const& rhs) {
	os << "RoutingDecision:\n";

	os << "- Matching server:\n";
	os << "  - root: '" << rhs.getServer()->getRoot() << "'\n";

	os << "- Matching location:";
	LocationBlock const* location = rhs.getLocation();
	if (location) os << "\n" << *location;
	else os << "[empty]";
	os << "\n";
	return os;
}
