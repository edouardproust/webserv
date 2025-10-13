#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "http/Request.hpp"
#include "config/ServerBlock.hpp"
#include "typedefs.hpp"

class Router
{
	Request const&					_request;
	std::vector<ServerBlock> const&	_servers;

	// Not used
	Router();
	Router(Router const&);
	Router&	operator=(Router const&);

	ServerBlock const& _findMatchingServer(std::string const& host, int port) const;
	LocationBlock const& _findMatchingLocation(ServerBlock const& server, std::string const& path) const;
	std::string _resolveScriptPath(std::string const& requestPath, LocationBlock const& location) const;
	std::string _resolveFilePath(std::string const& requestPath, LocationBlock const& location) const;

	public:

		Router(Request const&, std::vector<ServerBlock> const&);
		~Router();

		void	dispatchRequest() const;

		Request const&					getRequest() const;
		std::vector<ServerBlock> const&	getServers() const;

};

std::ostream&	operator<<(std::ostream&, Router const&);

#endif