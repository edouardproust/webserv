#ifndef STATICHANDLER_HPP
#define STATICHANDLER_HPP

#include "http/Response.hpp"
#include "config/LocationBlock.hpp"
#include "router/RoutingDecision.hpp"
#include "utils/utils.hpp"
#include "utils/typedefs.hpp"

/**
 * Handles serving static files for a HTTP request.
 *
 * Determines final path, MIME type, and generates response content.
 * Resource-type class: not copyable or assignable.
 */
class StaticHandler
{
	static size_t const		_FILE_MAX_SIZE;

	RoutingDecision const&	_routingDecision;
	LocationBlock const*	_location;

	std::string				_finalPath;

	Response _serveFile();

	std::string	_errorPageHtml(HttpStatus const& status) const;
	std::string	_welcomePageHtml() const;
	std::string	_autoindexHtml() const;
	std::string	_getCurrentDateLocal(time_t time) const;

	std::string	_getMime(const std::string& filePath);
	std::string	_getMimeFromPath(const std::string& filePath);

	// Default and copy constructors, assignation are forbidden
	StaticHandler();
	StaticHandler(const StaticHandler& other);
	StaticHandler&	operator=(StaticHandler const& other);

	public:

		StaticHandler(RoutingDecision const& rd);
		~StaticHandler();

		Response	handleGet();
		Response	handleDelete();
		Response	handleHead();
		Response	handlePut();
		Response	handlePost();
		Response	handleError(HttpStatus const& status);

		std::string const&	getFinalPath() const;
		bool	hasUpdatedFinalPath() const;
};

std::ostream& operator<<(std::ostream& os, StaticHandler const& rhs);

#endif
