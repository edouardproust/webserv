#include "router/RedirectionHandler.hpp"
#include "static/StaticHandler.hpp"

Response	RedirectionHandler::run(int code, std::string const& path)
{
	if (DEVMODE) {
		std::cout << "[DEBUG] RedirectionHandler:\n"
			<< "- handle: Redirection\n"
			<< "- code: " << code << "\n"
			<< "- path: " << path << std::endl;
	}
	if (path.empty())
		return StaticHandler::handleError(HttpStatus("internal_server_error"), NULL);
	Response response;
	response.setStatus(HttpStatus(code));
	if (code < 300 || code >= 400)
		response.setHeader("Location", path);
	response.setBody(StaticHandler::generateStatusHtml(HttpStatus(code)));
	return response;
}
