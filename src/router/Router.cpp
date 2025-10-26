#include "router/Router.hpp"

Router::Router() {}

Router::~Router() {}

/**
 * Dispatch the request to the corresponding handler by crossing data between Config and Request.
 *
 * Notes:
 * - Router does not check the validity of the data inside Config and Request objects,
 *   so the data needs to be 100% correct when passed to the Router. Related parsers are
 *   responsible for checking the data thoroughly.
 * - A POST request matching a non-CGI location returns an error (method_not_allowed)
 * - It is the role of StaticHandler to check if the requested file or directory is accesible.
 *   It also checks "autoindex" and "index" directives in config, in order to serve a directory
 *   listing page if needed.
 * - It is the role of CGIHandler to check if the script exists and is executable.
 */
Response Router::dispatchRequest(Config const& config, Request const& request, HostPortPair const& listen) {
	RoutingDecision rd(config, request, listen);
	if (DEVMODE) std::cout << rd << std::endl;
	RoutingDecision::Decision decision = rd.getDecision();
	LocationBlock const* loc = rd.getLocation();
	std::string const& locRoot = loc->getRoot();
	ErrorPages const& locErrorPages = loc->getErrorPages();

	// error, redirection
	HttpStatus const& reqStatus = request.getStatus();
	if (reqStatus.getSlug() != "ok")
		return StaticHandler::handleError(HttpStatus(reqStatus), locRoot, locErrorPages);

	if (decision == RoutingDecision::ERROR)
		return StaticHandler::handleError(HttpStatus(rd.getErrorSlug()), locRoot, locErrorPages);
	if (decision == RoutingDecision::REDIRECTION) {
		std::pair<int, std::string> const& ret = loc->getReturn();
		return RedirectionHandler::handleRedirection(ret.first, ret.second);
	}
	// cgi, static
	std::string path = _buildFilePath(loc->getRoot(), loc->getPath(), request.getPath());
	if (decision == RoutingDecision::CGI) {
		std::string ext = utils::getFileExtension(path);
		return CGIHandler::handleRequest(request, path, loc->getCgiExecutor(ext));
	}
	if (decision == RoutingDecision::STATIC) {
		std::string const&	method = request.getMethod();
		if (method == "GET")
			return StaticHandler::handleGet(request, path, loc->getAutoindex() == "on", loc->getIndexFiles());
		else if (method == "DELETE")
			return StaticHandler::handleDelete(request, path);
		// -- additional supported methods can be added here -- // TODO PUT method
		else
			return StaticHandler::handleError(HttpStatus("method_not_allowed"), locRoot, locErrorPages);
	}
	// fallback
	return StaticHandler::handleError(HttpStatus("internal_server_error"), locRoot, locErrorPages);
}

std::string	Router::_buildFilePath(std::string const& locRoot, std::string const& locPath, std::string const& reqPath) {
	std::string relativeReqPath = reqPath;
	if (reqPath.find(locPath) == 0)
		relativeReqPath = reqPath.substr(locPath.length());
	if (relativeReqPath.empty()) relativeReqPath = "/";
	std::string joinedPath = utils::joinPath(locRoot, relativeReqPath);
	return utils::normalizePath(joinedPath);
}
