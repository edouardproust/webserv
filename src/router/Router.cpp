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
	Response resp;
	if (DEVMODE)
		std::cout << rd << std::endl;
	RoutingDecision::Decision decision = rd.getDecision();
	LocationBlock const* loc = rd.getLocation();
	std::string const& finalPath = rd.getFinalPath();
	// Request parser Bad request
	StaticHandler statiq(rd);
	HttpStatus const& reqStatus = req.getStatus();
	if (reqStatus.getSlug() != "ok")
		resp = statiq.handleError(HttpStatus(reqStatus));
	// Routing decision: ERROR or REDIRECTION
	else if (decision == RoutingDecision::ERROR)
		resp = statiq.handleError(HttpStatus(rd.getErrorSlug()));
	else if (decision == RoutingDecision::REDIRECTION) {
		std::pair<int, std::string> const& ret = loc->getReturn();
		resp = RedirectionHandler::run(ret.first, ret.second);
	}
	// Routing decision: CGI
	else if (decision == RoutingDecision::CGI) {
		CgiHandler cgi;
		try {
			resp = cgi.run(req, loc, finalPath);
		} catch (CgiHandler::ExecException& e) {
			std::cerr << "[WARNING] CGI (" << finalPath << "): " << e.what() << std::endl;
			resp = statiq.handleError(HttpStatus("internal_server_error"));
		} catch (Response::RawException& e) {
			std::cerr << "[WARNING] CGI (" << finalPath << "): " << e.what() << std::endl;
			resp = statiq.handleError(HttpStatus("bad_gateway"));
		}
	}
	// Routing decision: STATIC
	else if (decision == RoutingDecision::STATIC) {
		std::string const&	method = req.getMethod();
		if (method == "GET")
			resp = statiq.handleGet();
		else if (method == "DELETE")
			resp = statiq.handleDelete();
		// -- additional supported methods can be added here -- // TODO PUT method
		else
			resp = statiq.handleError(HttpStatus("method_not_allowed"));
	}
	// fallback
	else
		resp = statiq.handleError(HttpStatus("internal_server_error"));

	if (DEVMODE && statiq.hasUpdatedFinalPath())
		std::cout << "StaticHandler:\n" << statiq << std::endl;
	return resp;
}
