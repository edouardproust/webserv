#ifndef DEV_HTTP_HPP
#define DEV_HTTP_HPP

#include "constants.hpp"
#include "http/Request.hpp"

namespace dev {

	Request const&	parseRequest(std::string const& rawRequest);
	std::string		getStatusString(::ParseStatus);
	void			runParserTests();

}

#endif