#ifndef REDIRECTION_HANDLER_HPP
#define REDIRECTION_HANDLER_HPP

#include "http/Response.hpp"
#include <string>
#include <iostream> // DEBUG

class RedirectionHandler {

	// Not used
	RedirectionHandler();
	RedirectionHandler(RedirectionHandler const&);
	RedirectionHandler&	operator=(RedirectionHandler const&);
	~RedirectionHandler();

	public:

	static Response	handleRedirection(int code, std::string const& path);

};

#endif