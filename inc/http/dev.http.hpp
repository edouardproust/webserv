#ifndef DEV_HTTP_HPP
#define DEV_HTTP_HPP

#include "constants.hpp"

namespace dev {

	const char*	parseStatusToString(::ParseStatus);
	void		runParserTests();

}

#endif