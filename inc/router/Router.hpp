#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "router/RoutingDecision.hpp"
#include "router/RedirectionHandler.hpp"
#include "static/StaticHandler.hpp"
#include "cgi/CgiHandler.hpp"
#include "colors.hpp"

class Router
{

	static void	_handleRedirectionDecision(Response&, RoutingDecision const&);
	static void	_handleCgiDecision(Response&, RoutingDecision const&, StaticHandler&);
	static void	_handleStaticDecision(Response&, std::string const&, StaticHandler&);

	// TODO make canonical
	Router();
	Router(Router const&);
	Router&	operator=(Router const&);
	~Router();

	public:

		static Response 	dispatchRequest(Config const&, Request const&, HostPortPair const&);

};

#endif