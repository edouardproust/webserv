#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "router/RoutingDecision.hpp"
#include "router/RedirectionHandler.hpp"
#include "static/StaticHandler.hpp"
#include "cgi/CgiHandler.hpp"

/**
 * Routes incoming HTTP requests to the appropriate handler.
 *
 * Dispatches requests to redirection, static, or CGI handlers based on server configuration.
 * Static service-type class: not instantiable, copyable, or assignable.
 */
class Router
{
	static void	_handleRedirectionDecision(Response&, RoutingDecision const&);
	static void	_handleCgiDecision(Response&, RoutingDecision const&, StaticHandler&);
	static void	_handleStaticDecision(Response&, std::string const&, StaticHandler&);

	// Not instantiable
	Router();
	Router(Router const&);
	Router&	operator=(Router const&);
	~Router();

	public:

		static Response 	dispatchRequest(Config const&, Request const&, HostPortPair const&);
};

#endif