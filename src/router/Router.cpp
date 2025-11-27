#include "router/Router.hpp"

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
Response Router::dispatchRequest(Config const& config, Request const& req, HostPortPair const& listeningOn)
{
	RoutingDecision rd(config, req, listeningOn);
	Log::dev("debug", "Routing Decision:\n" + utils::str(rd));

	Response resp;
	RoutingDecision::Decision decision = rd.getDecision();

	if (req.getStatus().getSlug() != "ok")
		resp = StaticHandler::handleError(req.getStatus().getSlug(), rd);
	else if (decision == RoutingDecision::ERROR)
		resp = StaticHandler::handleError(rd.getErrorSlug(), rd);
	else if (decision == RoutingDecision::REDIRECTION)
		_handleRedirectionDecision(resp, rd);
	else if (decision == RoutingDecision::CGI)
		_handleCgiDecision(resp, rd, listeningOn);
	else if (decision == RoutingDecision::STATIC) {
		_handleStaticDecision(resp, req.getMethod());
	} else // fallback
		resp = StaticHandler::handleError("internal_server_error", rd);

	resp.setConnectionFromRequest(req);
	return resp;
}

void	Router::_handleRedirectionDecision(Response& resp, RoutingDecision const& rd)
{
	std::pair<int, std::string> const& ret = rd.getLocation()->getReturn();
	RedirectionHandler redirect(rd, ret.first, ret.second);
	resp = redirect.run();
}

void	Router::_handleCgiDecision(Response& resp, RoutingDecision const& rd, HostPortPair const& listeningOn)
{
	std::string const& scriptName = rd.getFinalPath();
	if (!utils::fileExists(scriptName)) {
		resp = StaticHandler::handleError("not_found", rd);
	} else if (!utils::isReadableFile(scriptName)) {
		resp = StaticHandler::handleError("forbidden", rd);
	} else {
		CgiParams cgiParams(rd.getRequest(), rd.getLocation(), scriptName, listeningOn);
		if (!cgiParams.isValid())
			resp = StaticHandler::handleError("not_implemented", rd);
		CgiHandler cgi(rd.getRequest(), cgiParams);
		try {
			resp = cgi.execute();
		} catch (CgiHandler::ExecException& e) {
			Log::prod("warning", "CGI execution error: " + scriptName + ": " + e.what());
			resp = StaticHandler::handleError("internal_server_error", rd);
		} catch (Response::RawException& e) {
			Log::prod("warning", "CGI invalid raw output: " + scriptName + ": " + e.what());
			resp = StaticHandler::handleError("bad_gateway", rd);
		} catch (CgiHandler::TimeoutException& e) {
			Log::prod("warning", "CGI timeout: " + scriptName + ": " + e.what());
			resp = StaticHandler::handleError("timeout", rd);
		}
        Log::dev("debug", "CGI Handler:\n" + utils::str(cgi));
	}
}

void	Router::_handleStaticDecision(Response& resp, std::string const& method)
{
	if (method == "GET")
		resp = statiq.handleGet();
	else if (method == "DELETE")
		resp = statiq.handleDelete();
	else if (method == "HEAD")
		resp = statiq.handleHead();
	else if (method == "PUT")
		resp = statiq.handlePut();
	else if (method == "POST")
		resp = statiq.handlePost();
	// -- additional supported methods can be added here --
	else
		resp = StaticHandler::handleError("method_not_allowed", rd);
}
