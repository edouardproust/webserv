#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "http/Request.hpp"
#include "config/ServerBlock.hpp"
#include "typedefs.hpp"

class Router
{
	Request const&					_request;
	std::vector<ServerBlock> const&	_servers;

	Router(); // Not used

	public:

		Router(Request const&, std::vector<ServerBlock> const&);
		~Router();

		void	resolve();
		void	print();

};

#endif