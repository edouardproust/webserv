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
Response	Router::dispatchRequest(Config const& config, Request const& req, HostPortPair const& listeningOn)
{
	RoutingDecision rd(config, req, listeningOn);
	Log::dev("debug", "Routing Decision:\n" + utils::str(rd));

	RoutingDecision::Decision decision = rd.getDecision();

	if (req.getStatus().getSlug() != "ok")
		return StaticHandler::error(req.getStatus().getSlug(), rd);
	if (decision == RoutingDecision::ERROR)
		return StaticHandler::error(rd.getErrorSlug(), rd);
	if (decision == RoutingDecision::REDIRECTION)
		return _handleRedirectionDecision(rd);
	if (decision == RoutingDecision::CGI)
		return _handleCgiDecision(rd, listeningOn);
	if (decision == RoutingDecision::STATIC)
		return _handleStaticDecision(rd);
	return StaticHandler::error("internal_server_error", rd);
}

Response	Router::_handleRedirectionDecision(RoutingDecision const& rd)
{
	std::pair<int, std::string> const& ret = rd.getLocation()->getReturn();
	RedirectionHandler redirect(rd, ret.first, ret.second);
	return redirect.run();
}

Response	Router::_handleCgiDecision(RoutingDecision const& rd, HostPortPair const& listeningOn)
{
	std::string const& scriptName = rd.getFinalPath();
	if (!utils::fileExists(scriptName))
		return StaticHandler::error("not_found", rd);
	if (!utils::isReadableFile(scriptName))
		return StaticHandler::error("forbidden", rd);

	CgiParams cgiParams(rd.getRequest(), rd.getLocation(), scriptName, listeningOn);
	if (!cgiParams.isValid())
		return StaticHandler::error("not_implemented", rd);
	return Response::createCgiResponse(cgiParams, rd);
}

Response	Router::_handleStaticDecision(RoutingDecision const& rd)
{
	std::string const& method = rd.getRequest().getMethod();

	if (method == "GET")
		return StaticHandler::get(rd);
	if (method == "DELETE")
		return StaticHandler::del(rd);
	if (method == "HEAD")
		return StaticHandler::head(rd);
	if (method == "PUT")
		return StaticHandler::put(rd);
	if (method == "POST")
		return StaticHandler::post(rd);
	// -- additional supported methods can be added here --
	return StaticHandler::error("method_not_allowed", rd);
}
