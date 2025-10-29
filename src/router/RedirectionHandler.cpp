#include "router/RedirectionHandler.hpp"

Response	RedirectionHandler::run(int code, std::string const& path) {
	// TODO
	std::cout << "[DEBUG] RedirectionHandler:\n"
		<< "- handle: Redirection\n"
		<< "- code: " << code << "\n"
		<< "- path: " << path << std::endl;
	// Dummy response
	Response response;
	response.setError(code);
	return response;
}
