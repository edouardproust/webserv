#include "router/Router.hpp"
#include "static/StaticHandler.hpp"
#include "cgi/CGIHandler.hpp"
#include "utils/utils.hpp"
#include "constants.hpp"
#include <iostream>

Router::Router(Request const& request, std::vector<ServerBlock> const& servers, HostPortPair const& listeningOn)
: _request(request), _servers(servers), _listeningOn(listeningOn), _matchingLocation(NULL) {}

Router::~Router() {}

void Router::dispatchRequest() {
    ParseStatus requestStatus = _request.getStatus();
    if (requestStatus != PARSE_SUCCESS) {
        StaticHandler::sendErrorPageContent(requestStatus);
        return;
    }
    ServerBlock const& server = _findMatchingServer();
    _setMatchingLocation(server.getLocations(), _request.getPath());
	if (!_matchingLocation)
		_matchingLocation = &server.getDefaultLocation();

	/* Checks

	Resume:
	return → rediriger si présent.
	1. limit_except → si méthode non autorisée → 405 (avec Allow).
	2. client_max_body_size → si Content-Length connu et > limit → 413. Si chunked → parser gère.
	3. Path normalization & security → rejeter si resolved path sort du root → 403.
	4. Décider CGI vs Static (extension mapping) ; si CGI attendu mais executor manquant, 500 ou config-time fail.
	5. Laisser StaticHandler faire stat, index search, autoindex, permissions et renvoyer 200/404/403 selon cas.
	6. CGIHandler s’occupe du fork/exec, et doit renvoyer 500 si execve échoue.

	// 1. Redirection
	if (!_matchingLocation.getReturn().empty())
		sentResponse(3xx with Location: (status and target));

	// 2. Unauthorized Method
	std::set<std::string> limitExcept = _matchingLocation.getLimitExcept();
	if (!limitExcept.empty() && !limitExcept.find(reauest.getMethod()))
		sendRespose(405 Method Not Allowed + header Allow: <liste>);

	// 3. File too large
		Taille du body (client_max_body_size)
		- Si Request a une Content-Length (ou le parser sait la longueur effective) et que length > limit → 413 Payload Too Large.
		- Si Transfer-Encoding: chunked géré par le parser, vérifier la taille totale pendant la lecture si possible et appliquer la même limite.

	// 4. Path normalization & sécurité
		- Concaténer le root + normalizedPath → candidate.
		- Obtenir le chemin canonique du candidate
		- Vérifier que le chemin résolu ne sort pas du root (prévenir .. / path traversal).
		- Vérifier que le chemin canonique commence par le chemin canonique du root
		- Attention aux symlinks
		- Si résolution mène hors root → 403 Forbidden.

	// 5. Autoindex
		- Si pas d'index et `autoindex:on` Produire page de listing des fichiers
		- Si pas d index et `autoindex:off ou empty: reponse 403/404 (selon config)

	// 6. Existence / permissions de la ressource (optionnel dans Router)
		- Idéal : le Router ne lit pas les fichiers, laisse StaticHandler faire stat(); mais le Router peut décider
			- si le path se termine par / → il peut attendre qu’on recherche des index (ou demander StaticHandler de le faire).
		- Toutefois le Router peut décider d’envoyer directement un code 405/403 avant d’appeler StaticHandler si la méthode est incompatible.
	*/

	// 6. CGI
    std::string executor;
    if (_matchingLocation->isCgiLocation()) { // the request path matches a CGI location
        executor = _matchingLocation->getCgiExecutor(utils::getFileExtension(_request.getPath()));
    }
	if (!executor.empty()) { // the file extension is handled by the CGI config of this location
        std::string scriptPath = _resolveScriptPath(_request.getPath(), _matchingLocation);
        std::string executor = _matchingLocation->getCgiExecutor(utils::getFileExtension(_request.getPath()));
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
		std::cout << "DEBUGG" << std::endl;

        std::string filePath = _resolveFilePath(_request.getPath(), _matchingLocation);
        StaticHandler::sendStaticContent(
            filePath,
            _request.getMethod(),
            _request.getHeaders()
        );
    }

	/*
	Checks to do in CGIHandler:
	- CGIHandler:
		- If execve fails (invalid executable path): send back 500 response
	- StaticHandler:
		- File size > matchingLocation.getClientMaxBodySize();
	*/

	/*
	AVA work:
	- Verifier que path n est pas trop long (et stocker la valeur de PATH_MAX dans constants.cpp)
	- Calculer et checker la taille du body de la raw request et la stocker dans Request::_bodySize
	- Idem si on a "Transfer-Encoding: chunked" ? -> Additionner la taille de chaque chunk
	- Requête : GET /images/%2e%2e/%2e%2e/etc/passwd (encodage URL) -> il faut décoder et normaliser:
		- Décoder %xx (URL decode)
		- Remplacer // par /, résoudre . et .. segments (removal of dot-segments).
	-  Checker présence/validité de Content-Length
		- Pour méthodes qui requièrent body (PUT/POST) : si Content-Length absent et pas de chunked → 411 Length Required (ou 400 selon ta politique), mais souvent parser gère ça plus haut.
	*/

	/* Daniel work:
	- Checker le cas ou on a un "Transfert-encoding: chuncked": les donnees arrivent de maniere differente: chaque block commence par la taille hexadecimal puis un bloc de taille 0 marque la fin.
	*/
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
	// TODO filter by Request::method + LocationBlock->getLimitExcept()
	_matchingLocation = best;
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
	return _matchingLocation;
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
	os << "- Matching location:\n";
	if (matchingLoc) os << *matchingLoc;
	else os << "No match";
	return os;
}
