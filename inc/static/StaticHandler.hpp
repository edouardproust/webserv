#ifndef STATIQ_HPP
#define STATIQ_HPP

#include "constants.hpp"
#include "http/Request.hpp"
#include "http/Response.hpp"
#include "http/HttpStatus.hpp"
#include "utils/utils.hpp"
#include <string>
#include <map>
#include <vector>

class StaticHandler {

	// Not used
	StaticHandler();
	StaticHandler(StaticHandler const&);
	~StaticHandler();
	StaticHandler&	operator=(StaticHandler const&);

	public:

		static Response	handleRequest(std::string const&, Request const&);
		static Response	handleError(HttpStatus const&);

};

#endif