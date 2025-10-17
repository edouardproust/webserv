#ifndef DEV_HTTP_HPP
#define DEV_HTTP_HPP

#include "constants.hpp"
#include "http/Request.hpp"
#include "http/RequestParser.hpp"
#include "http/Response.hpp"

namespace dev {

	Request const&	parseRequest(std::string const& rawRequest);
	void			runParserValidationTests();
	void			runParsedContentTests();
	void 			runResponseTests();
}

#endif
