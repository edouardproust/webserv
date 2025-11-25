#ifndef ROUTING_DECISION_HPP
#define ROUTING_DECISION_HPP

#include "config/Config.hpp"

/**
 * Determines routing for a HTTP request.
 *
 * Captures server, location, final path, and decision type.
 * Resource-type class: not copyable or assignable.
 */
class RoutingDecision
{
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
		HostPortPair const&		_listeningOn;
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

		// Copy constructors, assignation are forbidden
		RoutingDecision(RoutingDecision const&);
		RoutingDecision&	operator=(RoutingDecision const&);

	public:

		RoutingDecision();
		RoutingDecision(Config const&, Request const&, HostPortPair const&);
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