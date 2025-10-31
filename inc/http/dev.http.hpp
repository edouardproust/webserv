#ifndef DEV_PARSE_HPP
#define DEV_PARSE_HPP

#include "constants.hpp"
#include "HttpStatus.hpp"
#include "Request.hpp"
#include "RequestParser.hpp"
#include "Response.hpp"
#include "static/StaticHandler.hpp"

namespace dev {

	Request const&	parseRequest(std::string const& rawRequest);
	void			runParserValidationTests();
	void			runParsedContentTests();
	void 			runResponseTests();
}

#endif
