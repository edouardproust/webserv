#ifndef STATICHANDLER_HPP
#define STATICHANDLER_HPP

#include "http/Response.hpp"
#include "config/LocationBlock.hpp"
#include "router/RoutingDecision.hpp"
#include "utils/utils.hpp"
#include "typedefs.hpp"

class StaticHandler
{
	RoutingDecision const&	_routingDecision;
	LocationBlock const*	_location;

	std::string				_finalPath;

	Response _serveFile();

	std::string	_errorPageHtml(HttpStatus const& status) const;
	std::string	_welcomePageHtml() const;
	std::string	_autoindexHtml() const;
	std::string	_getCurrentDateLocal(time_t time);

	std::string	_getMime(const std::string& filePath);
	std::string	_getMimeFromPath(const std::string& filePath);


	// TODO make canonical
	StaticHandler();
	StaticHandler(const StaticHandler& other);
	StaticHandler&	operator=(StaticHandler const& other);

	public:

		StaticHandler(RoutingDecision const& rd);
		~StaticHandler();

		Response	handleGet();
		Response	handleDelete();
		//Response	handlePut(std::string const&, Request const&);
		Response	handleError(HttpStatus const& status);

		std::string const&	getFinalPath() const;
		bool	hasUpdatedFinalPath() const;

};

std::ostream& operator<<(std::ostream& os, StaticHandler const& rhs);

#endif
