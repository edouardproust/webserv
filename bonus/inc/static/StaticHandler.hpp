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

	static std::string	_welcomePageHtml();
	std::string			_autoindexHtml() const;
	static std::string	_errorPageHtml(HttpStatus const&);


	static std::string	_getMime(std::string const&);
	static std::string	_getMimeFromPath(std::string const&);

	// Default and copy constructors, assignation are forbidden
	StaticHandler();
	StaticHandler(const StaticHandler&);
	StaticHandler&	operator=(StaticHandler const&);

	public:

		StaticHandler(RoutingDecision const&);
		~StaticHandler();

		Response		handleGet();
		Response		handleDelete();
		Response		handleHead();
		Response		handlePut();
		Response		handlePost();
		Response		handleError(HttpStatus const&);
		static Response	handleError(std::string const&);

		std::string const&	getFinalPath() const;
		bool	hasUpdatedFinalPath() const;
};

std::ostream& operator<<(std::ostream&, StaticHandler const&);

#endif
