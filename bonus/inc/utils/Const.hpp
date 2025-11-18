#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <limits>
#include <string>
#include <cstddef>

#ifndef DEVMODE
# define DEVMODE 0
#endif

class Const
{
	public:

		static size_t const			MAX_SIZE_T;

		static std::string const	SERVER_NAME;
		static std::string const	SERVER_VERSION;
		static std::string const	SERVER_REPO;
		static std::string const	SERVER_SOFTWARE;
};

#endif
