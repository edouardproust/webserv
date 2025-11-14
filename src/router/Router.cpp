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

	Response resp;
	StaticHandler statiq(rd);
	RoutingDecision::Decision decision = rd.getDecision();

	if (req.getStatus().getSlug() != "ok")
		resp = statiq.handleError(HttpStatus(req.getStatus()));
	else if (decision == RoutingDecision::ERROR)
		resp = statiq.handleError(HttpStatus(rd.getErrorSlug()));
	else if (decision == RoutingDecision::REDIRECTION)
		_handleRedirectionDecision(resp, rd);
	else if (decision == RoutingDecision::CGI)
		_handleCgiDecision(resp, rd, statiq);
	else if (decision == RoutingDecision::STATIC) {
		_handleStaticDecision(resp, req.getMethod(), statiq);
	} else // fallback
		resp = statiq.handleError(HttpStatus("internal_server_error"));

	resp.setConnectionFromRequest(req);
	if (DEVMODE && statiq.hasUpdatedFinalPath())
		std::cout << "StaticHandler:\n" << statiq << std::endl;
	return resp;
}

void	Router::_handleRedirectionDecision(Response& resp, RoutingDecision const& rd) {
	std::pair<int, std::string> const& ret = rd.getLocation()->getReturn();
	RedirectionHandler redirect(rd, ret.first, ret.second);
	resp = redirect.execute();
}

void	Router::_handleCgiDecision(Response& resp, RoutingDecision const& rd, StaticHandler& statiq) {
	std::string const& scriptName = rd.getFinalPath();
	if (!utils::fileExists(scriptName)) {
		resp = statiq.handleError(HttpStatus("not_found"));
	} else if (!utils::isReadableFile(scriptName)) {
		resp = statiq.handleError(HttpStatus("forbidden"));
	} else {
		CgiHandler cgi;
		try {
			resp = cgi.run(rd.getRequest(), rd.getLocation(), scriptName);
		} catch (CgiHandler::ExecException& e) {
			std::cerr << FT_WARNING << "CGI execution error: " << scriptName << ": " << e.what() << std::endl;
			resp = statiq.handleError(HttpStatus("internal_server_error"));
		} catch (Response::RawException& e) {
			std::cerr << FT_WARNING << "CGI invalid raw output: " << scriptName << ": " << e.what() << std::endl;
			resp = statiq.handleError(HttpStatus("bad_gateway"));
		} catch (CgiHandler::TimeoutException& e) {
			std::cerr << FT_WARNING << "CGI timeout :" << scriptName << ": " << e.what() << std::endl;
			resp = statiq.handleError(HttpStatus("timeout"));
		}
	}
}

void	Router::_handleStaticDecision(Response& resp, std::string const& method, StaticHandler& statiq) {
	if (method == "GET")
		resp = statiq.handleGet();
	else if (method == "DELETE")
		resp = statiq.handleDelete();
	else if (method == "HEAD")
		resp = statiq.handleHead();
	else if (method == "PUT")
		resp = statiq.handlePut();
	// -- additional supported methods can be added here --
	else
		resp = statiq.handleError(HttpStatus("method_not_allowed"));
}
