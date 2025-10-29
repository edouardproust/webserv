#include "static/StaticHandler.hpp"

/**
 * Else, build a standard error page with `HttpStatus->toString()`.
 */
Response	StaticHandler::error(HttpStatus const& status, std::string const& locRoot, ErrorPages const& locErrorPages) {
	std::cout << "[DEBUG] StaticHandler:\n"
		<< "- action: Error\n"
		<< "- status: " << status.toString() << "\n"
		<< "- location root: " << locRoot << "\n"
		<< "- error pages: " << locErrorPages.size() << "\n";
	for (ErrorPages::const_iterator it = locErrorPages.begin(); it != locErrorPages.end(); ++it)
		std::cout << "  - " << it->first << " -> " << it->second << "\n";
	std::cout << std::endl;

	// If one of `locErrorPages` corresponds to `HttpStatus->getCode()`, get its content.
	ErrorPages::const_iterator search = locErrorPages.find(status.getCode());
	if (search != locErrorPages.end()) {
		std::string errorPath = utils::joinPath(locRoot, search->second);
		// if errorPath exists and is a readable file: return a response with its content
		// else return a Reponse with content of a built error page (not_found or forbidden)
	}
	// return a Response with the content of a built error page
	Response response;
	response.setStatus(status);
	return response;
}

Response	StaticHandler::get(Request const& request, std::string const& path, bool isAutoindex, std::vector<std::string> const& locIndexes) {
	std::cout << "[DEBUG] StaticHandler:\n"
		<< "- handle: Get\n"
		<< "- method: " << request.getMethod() << "\n"
		<< "- file path: " << path << "\n"
		<< "- autoindex: " << (isAutoindex ? "true" : "false") << "\n"
		<< "- location indexes: " << locIndexes.size() << "\n";
	for (size_t i = 0; i < locIndexes.size(); ++i)
		std::cout << "  - " << locIndexes[i] << "\n";
	std::cout << std::endl;

	/*
	if path is a directory:
		if isAutoIndex: loop over indexes and build indexPath = utils::joinPath(path, indexes[i]). If a indexPath is a readable file: return a Response with its content
		else: return a Reponse with the content of a built directory index page
	else if is a regular file:
		if is an existing and readable file: return a Response with its content
		else: return a Response with a built error page (not_found or forbidden)
	*/
	Response response;
	return response;
}

Response	StaticHandler::del(Request const& request, std::string const& path) {
	std::cout << "[DEBUG] StaticHandler:\n"
		<< "- handle: Delete\n"
		<< "- method: " << request.getMethod() << "\n"
		<< "- file path: " << path << "\n"
		<< std::endl;

	Response response;
	return response;
}

