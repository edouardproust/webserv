#ifndef STATIQ_HPP
#define STATIQ_HPP

#include "constants.hpp"
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

		static void	sendErrorPageContent(ParseStatus status);
		static void	sendStaticContent(
			std::string const& filePath,
			std::string const& method,
			std::map<std::string, std::string> const& headers
		);

};

#endif