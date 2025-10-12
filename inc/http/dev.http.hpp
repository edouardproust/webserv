#ifndef DEV_HTTP_HPP
#define DEV_HTTP_HPP

#include "constants.hpp"
#include "http/Request.hpp"
#include "http/RequestParser.hpp"

namespace dev {

	std::string		parseStatusToString(ParseStatus);
	void			runParserTests();

}

#endif