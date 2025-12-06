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
 * Static service-type class: not instantiable.
 */
class StaticHandler
{
	static size_t const	_FILE_MAX_SIZE;

	static Response		_serveFile(std::string const&, RoutingDecision const&);
	static std::string	_builtinAutoindexHtml(RoutingDecision const&);
	static std::string	_builtinErrorPageHtml(HttpStatus const&);
	static std::string	_builtinWelcomePageHtml();

	static std::string	_getMime(std::string const&);
	static std::string	_getMimeFromPath(std::string const&);

	// Not instantiable
	StaticHandler();
	~StaticHandler();
	StaticHandler(const StaticHandler&);
	StaticHandler&	operator=(StaticHandler const&);

	public:

		static Response	get(RoutingDecision const&);
		static Response	del(RoutingDecision const&);
		static Response	head(RoutingDecision const&);
		static Response	put(RoutingDecision const&);
		static Response	post();
		static Response	error(std::string const&, RoutingDecision const&);
		static Response	error(std::string const&, Request const&, ErrorPages const&);

		static Response	builtinError(std::string const&, std::string const&);
};

#endif
