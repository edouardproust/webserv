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
	std::string const& filePath = rd.getFilePath();

	// Request parser Bad request
	HttpStatus const& reqStatus = req.getStatus();
	if (reqStatus.getSlug() != "ok")
		return StaticHandler::handleError(HttpStatus(reqStatus), loc);
	// Transversal attack check
	if (filePath.empty())
		return StaticHandler::handleError(HttpStatus("forbidden"), loc);
	// Routing Decision error or redirection
	if (decision == RoutingDecision::ERROR)
		return StaticHandler::handleError(HttpStatus(rd.getErrorSlug()), loc);
	if (decision == RoutingDecision::REDIRECTION) {
		std::pair<int, std::string> const& ret = loc->getReturn();
		return RedirectionHandler::run(ret.first, ret.second);
	}
	// cgi, static
	if (decision == RoutingDecision::CGI) {
		CgiHandler handler;
		try {
			return handler.run(req, loc, filePath);
		} catch (CgiHandler::ExecException& e) {
			std::cerr << "[WARNING] CGI (" << filePath << "): " << e.what() << std::endl;
			return StaticHandler::handleError(HttpStatus("internal_server_error"), loc);
		} catch (Response::RawException& e) {
			std::cerr << "[WARNING] CGI (" << filePath << "): " << e.what() << std::endl;
			return StaticHandler::handleError(HttpStatus("bad_gateway"), loc);
		}
	}
	if (decision == RoutingDecision::STATIC) {
		std::string const&	method = req.getMethod();
		if (method == "GET")
			return StaticHandler::handleGet(filePath, loc);
		else if (method == "DELETE")
			return StaticHandler::handleDelete(filePath, loc);
		// -- additional supported methods can be added here -- // TODO PUT method
		else
			return StaticHandler::handleError(HttpStatus("method_not_allowed"), loc);
	}
	// fallback
	return StaticHandler::handleError(HttpStatus("internal_server_error"), loc);
}
