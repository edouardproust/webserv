#ifndef STATICHANDLER_HPP
#define STATICHANDLER_HPP

#include "constants.hpp"
#include "http/Request.hpp"
#include "http/Response.hpp"
#include <string>
#include <map>

class StaticHandler
{

	// Not used
	StaticHandler();
	StaticHandler(StaticHandler const&);
	~StaticHandler();
	StaticHandler&	operator=(StaticHandler const&);

	public:

		static Response	handleRequest(std::string const&, Request const&);
		static Response	handleError(ParseStatus);

};

#endif
