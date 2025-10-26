#ifndef STATIQ_HPP
#define STATIQ_HPP

#include "http/Response.hpp"
#include "utils/utils.hpp"
#include "typedefs.hpp"

class StaticHandler {

	// Not used
	StaticHandler();
	StaticHandler(StaticHandler const&);
	~StaticHandler();
	StaticHandler&	operator=(StaticHandler const&);

	public:

		static Response	handleError(HttpStatus const&, std::string const&, ErrorPages const&);
		static Response	handleGet(Request const&, std::string const&, bool, std::vector<std::string> const&);
		static Response	handleDelete(Request const&, std::string const&);

};

#endif