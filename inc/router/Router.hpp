#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "http/Request.hpp"
#include "config/ServerBlock.hpp"
#include "typedefs.hpp"

class Router
{
	Request const&					_request;
	std::vector<ServerBlock> const&	_servers;
	HostPortPair const&				_listeningOn;
	LocationBlock const*			_location; // matching LocationBlock based on Request + Config

	void		_setMatchingLocation(std::vector<LocationBlock> const&, std::string const&);
	std::string	_resolveScriptPath(std::string const&, LocationBlock const*) const;
	std::string _resolveFilePath(std::string const&, LocationBlock const*) const;

	bool		_isRedirectionLocation() const;

	// Not used
	Router();
	Router(Router const&);
	Router&	operator=(Router const&);

	public:

		Router(Request const&, std::vector<ServerBlock> const&, HostPortPair const&);
		~Router();

	ServerBlock const& _findMatchingServer() const;

		void	dispatchRequest();

		Request const&					getRequest() const;
		std::vector<ServerBlock> const&	getServers() const;
		HostPortPair const&				getListeningOn() const;
		LocationBlock const*			getMatchingLocation() const;

};

std::ostream&	operator<<(std::ostream&, Router const&);

#endif