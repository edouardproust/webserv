#ifndef ROUTING_DECISION_HPP
#define ROUTING_DECISION_HPP

#include "config/Config.hpp"

class RoutingDecision {

	public:

	enum Decision {
		ERROR,
		REDIRECTION,
		STATIC,
		CGI,
	};

	private:

	// input
	Config const&			_config;
	Request const&			_request;
	HostPortPair const&		_listen;
	// process
	ServerBlock const*		_server;
	LocationBlock const*	_location;
	std::string 			_finalPath;
	// output
	Decision				_decision;
	std::string				_errorSlug;

	void	_makeDecision();

	void	_setServer();
	void	_setLocation();
	void	_setFinalPath();
	void	_setError(std::string const& errorSlug);

	// Not used // TODO canonical ?
	RoutingDecision();

	public:

	RoutingDecision(Config const&, Request const&, HostPortPair const&);
	RoutingDecision(RoutingDecision const&);
	RoutingDecision&	operator=(RoutingDecision const&);
	~RoutingDecision();

	Request const&			getRequest() const;
	ServerBlock const*		getServer() const;
	LocationBlock const*	getLocation() const;
	std::string const&		getFinalPath() const;
	Decision const&			getDecision() const;
	std::string const&		getErrorSlug() const;

};

std::ostream&	operator<<(std::ostream& os, RoutingDecision const& rhs);

#endif