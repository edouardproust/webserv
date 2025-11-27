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

	Response			_serveFile();
	std::string			_builtinAutoindexHtml() const;
	static Response		_buildErrorResponse(HttpStatus, std::string const&, std::string const&, ErrorPages const&);
	static std::string	_builtinErrorPageHtml(HttpStatus const&);
	static std::string	_builtinWelcomePageHtml();
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
		static Response	handleError(std::string const&, RoutingDecision const&);
		static Response	builtinError(std::string const&, std::string const&);

		std::string const&	getFinalPath() const;
};

#endif
