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
 * - It is the role of CgiHandler to check if the script exists and is executable.
 */
Response Router::dispatchRequest(Config const& config, Request const& req, HostPortPair const& listen) {
	RoutingDecision rd(config, req, listen);
	if (DEVMODE) std::cout << rd << std::endl;
	RoutingDecision::Decision decision = rd.getDecision();
	LocationBlock const* loc = rd.getLocation();
	std::string const& locRoot = loc->getRoot();
	ErrorPages const& locErrorPages = loc->getErrorPages();

	// error, redirection
	HttpStatus const& reqStatus = req.getStatus();
	if (reqStatus.getSlug() != "ok")
		return StaticHandler::error(HttpStatus(reqStatus), locRoot, locErrorPages);

	if (decision == RoutingDecision::ERROR)
		return StaticHandler::error(HttpStatus(rd.getErrorSlug()), locRoot, locErrorPages);
	if (decision == RoutingDecision::REDIRECTION) {
		std::pair<int, std::string> const& ret = loc->getReturn();
		return RedirectionHandler::run(ret.first, ret.second);
	}
	// cgi, static
	std::string filePath = _buildFilePath(loc->getRoot(), loc->getPath(), req.getPath());
	if (decision == RoutingDecision::CGI) {
		CgiHandler handler;
		try {
			return handler.run(req, loc, filePath);
		} catch (CgiHandler::ExecException& e) {
			std::cerr << "[WARNING] CGI (" << filePath << "): " << e.what() << std::endl;
			return StaticHandler::error(HttpStatus("internal_server_error"), locRoot, locErrorPages);
		} catch (Response::RawException& e) {
			std::cerr << "[WARNING] CGI (" << filePath << "): " << e.what() << std::endl;
			return StaticHandler::error(HttpStatus("bad_gateway"), locRoot, locErrorPages);
		}
	}
	if (decision == RoutingDecision::STATIC) {
		std::string const&	method = req.getMethod();
		if (method == "GET")
			return StaticHandler::get(req, filePath, loc->getAutoindex() == "on", loc->getIndexFiles());
		else if (method == "DELETE")
			return StaticHandler::del(req, filePath);
		// -- additional supported methods can be added here -- // TODO PUT method
		else
			return StaticHandler::error(HttpStatus("method_not_allowed"), locRoot, locErrorPages);
	}
	// fallback
	return StaticHandler::error(HttpStatus("internal_server_error"), locRoot, locErrorPages);
}

std::string	Router::_buildFilePath(std::string const& locRoot, std::string const& locPath, std::string const& reqPath) {
	std::string relativeReqPath = utils::trimDomain(reqPath);
	if (reqPath.find(locPath) == 0)
		relativeReqPath = reqPath.substr(locPath.length());
	if (relativeReqPath.empty()) relativeReqPath = "/";
	std::string joinedPath = utils::joinPath(locRoot, relativeReqPath);
	return utils::normalizePath(joinedPath);
}
