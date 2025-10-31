#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "router/RoutingDecision.hpp"
#include "router/RedirectionHandler.hpp"
#include "static/StaticHandler.hpp"
#include "cgi/CgiHandler.hpp"

class Router
{

	// TODO make canonical
	Router();
	Router(Router const&);
	Router&	operator=(Router const&);
	~Router();

	static std::string	_buildFilePath(std::string const&, std::string const&, std::string const&);

	public:

		static Response 	dispatchRequest(Config const&, Request const&, HostPortPair const&);

};

#endif