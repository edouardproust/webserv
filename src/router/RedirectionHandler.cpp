#include "router/RedirectionHandler.hpp"

Response	RedirectionHandler::run(int code, std::string const& path) {
	// TODO
	if (DEVMODE) {
		std::cout << "[DEBUG] RedirectionHandler:\n"
			<< "- handle: Redirection\n"
			<< "- code: " << code << "\n"
			<< "- path: " << path << std::endl;
	}
	// Dummy response
	Response response;
	response.setStatus(HttpStatus(code));
	response.setHeader("Location", path); // Needed : Location header for redirect
	return response;
}
