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

	public:

		Router(Request const&, std::vector<ServerBlock> const&);
		~Router();

		void	resolve() const;

		Request const&					getRequest() const;
		std::vector<ServerBlock> const&	getServers() const;

};

std::ostream&	operator<<(std::ostream&, Router const&);

#endif