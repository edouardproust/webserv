#ifndef ROUTING_DECISION_HPP
#define ROUTING_DECISION_HPP

#include "config/Config.hpp"
#include "http/Request.hpp"
#include "http/HttpStatus.hpp"
#include "config/ServerBlock.hpp"
#include "config/LocationBlock.hpp"

class RoutingDecision {

	Config const&			_config;
	Request const&			_request;
	HostPortPair const&		_listen;

	HttpStatus				_status;
	ServerBlock const*		_server;
	LocationBlock const*	_location;
	std::string				_finalPath;
	std::string				_cgiExecutor;
	std::string				_redirectTarget;

	void	_setServer();
	void	_setLocation();


	// Not used
	RoutingDecision();
	RoutingDecision(RoutingDecision const&);
	RoutingDecision&	operator=(RoutingDecision const&);

	void	_resolveFinalPath();

	public:

		RoutingDecision(Config const&, Request const&, HostPortPair const&);
		~RoutingDecision();

		void	setStatus();
		void	setFinalPath();
		void	setCgiExecutor();
		void	setRedirectTarget();

		ParseStatus				getStatus() const;
		ServerBlock const*		getServer() const;
		LocationBlock const*	getLocation() const;

};

std::ostream&	operator<<(std::ostream& os, RoutingDecision const& rhs);

#endif