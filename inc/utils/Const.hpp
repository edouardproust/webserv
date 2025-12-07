#ifndef CONST_HPP
#define CONST_HPP

#include <limits>
#include <string>
#include <cstddef>

#ifndef DEVMODE
# define DEVMODE 0
#endif

#ifndef UBUNTU_TESTER
# define UBUNTU_TESTER 0
#endif

#define PRINT_DEBUG 1
#define CGI_ENABLED 1

class Const
{
	public:

		static size_t const	MAX_SIZE_T;
		static size_t const	ABSOLUTE_MAX_CLIENT_BODY_SIZE;
		static size_t const	READ_WRITE_LOG_THRESHOLD_SIZE;

		static std::string const	SERVER_NAME;
		static std::string const	SERVER_VERSION;
		static std::string const	SERVER_REPO;
		static std::string const	SERVER_SOFTWARE;
};

#endif
