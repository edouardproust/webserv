#ifndef DEV_HTTP_HPP
#define DEV_HTTP_HPP

#include "Constants.hpp"
#include "http/Request.hpp"
#include "http/RequestParser.hpp"
#include "http/Response.hpp"

namespace dev {

	Request const&	parseRequest(std::string const&);
	std::string		parseStatusToString(ParseStatus);
	void			runParserTests();
	void 			runResponseTests();

}

#endif
