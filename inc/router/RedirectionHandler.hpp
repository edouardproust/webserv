#ifndef REDIRECTION_HANDLER_HPP
#define REDIRECTION_HANDLER_HPP

#include "http/Response.hpp"

class RedirectionHandler
{
	private:

	// Not used
	RedirectionHandler();
	RedirectionHandler(RedirectionHandler const&);
	RedirectionHandler&	operator=(RedirectionHandler const&);
	~RedirectionHandler();

	public:

	static Response	run(int code, std::string const& path);

};

#endif
