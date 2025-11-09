#ifndef REDIRECTION_HANDLER_HPP
#define REDIRECTION_HANDLER_HPP

#include "http/Response.hpp"
#include "router/RoutingDecision.hpp"

class RedirectionHandler
{
	private:

	RoutingDecision	_routingDecision;
	int				_code;
	std::string		_path;

	std::string	_generateRedirectionHtml() const;

	public:

	RedirectionHandler();
	RedirectionHandler(RoutingDecision const& rd, int code, std::string const& path)
	RedirectionHandler(RedirectionHandler const&);
	RedirectionHandler&	operator=(RedirectionHandler const&);
	~RedirectionHandler();

	Response	execute();

	int			getCode() const;
	std::string	const& getPath() const;

	void	setCode(int code);
	void	setPath(std::string const& path);

	static Response	run(RoutingDecision const& rd, int code, std::string const& path);

};

std::ostream& operator<<(std::ostream& os, RedirectionHandler const& rhs);

#endif
