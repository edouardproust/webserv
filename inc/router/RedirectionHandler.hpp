#ifndef REDIRECTION_HANDLER_HPP
#define REDIRECTION_HANDLER_HPP

#include "http/Response.hpp"
#include "router/RoutingDecision.hpp"

/**
 * Handles HTTP redirection responses.
 *
 * Generates appropriate status code and Location header.
 * Resource-type class: not copyable or assignable; manages redirection state.
 */
class RedirectionHandler
{
	RoutingDecision const&	_routingDecision;
	int						_code;
	std::string				_path;

	std::string	_RedirectionPageHtml() const;

	// Default and copy constructors, assignation is forbidden
	RedirectionHandler();
	RedirectionHandler(RedirectionHandler const&);
	RedirectionHandler&	operator=(RedirectionHandler const&);

	public:

		RedirectionHandler(RoutingDecision const&, int, std::string const&);
		~RedirectionHandler();

		Response	run();

		void	setCode(int);
		void	setPath(std::string const&);

		int					getCode() const;
		std::string	const&	getPath() const;
};

std::ostream&	operator<<(std::ostream&, RedirectionHandler const&);

#endif
