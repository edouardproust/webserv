#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "http/Request.hpp"
#include "http/Response.hpp"
#include "config/Config.hpp"
#include "typedefs.hpp"

class Router
{
	std::string	_resolveScriptPath(std::string const&, LocationBlock const*) const;
	std::string _resolveFilePath(std::string const&, LocationBlock const*) const;


	// Not used
	Router();
	Router(Router const&);
	Router&	operator=(Router const&);
	~Router();

	public:

		static Response 	dispatchRequest(Config const&, Request const&, HostPortPair const&);
		static void			sendResponse(Response const&);

};

#endif