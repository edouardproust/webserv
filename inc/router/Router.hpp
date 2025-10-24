#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "http/Request.hpp"
#include "http/HttpStatus.hpp"
#include "config/Config.hpp"
#include "router/RoutingDecision.hpp"
#include "static/StaticHandler.hpp"
#include "cgi/CGIHandler.hpp"

class Router
{
	std::string	_resolveScriptPath(std::string const&, LocationBlock const*) const;
	std::string _resolveFilePath(std::string const&, LocationBlock const*) const;


	// Not used
	Router();
	Router(Router const&);
	Router&	operator=(Router const&);
	~Router();

	static std::string	_buildFilePath(std::string const& locRoot, std::string const& locPath, std::string const& reqPath);

	public:

		static Response 	dispatchRequest(Config const&, Request const&, HostPortPair const&);

};

#endif