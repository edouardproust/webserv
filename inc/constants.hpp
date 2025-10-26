#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <limits>
#include <string>
#include <cstddef>

// global

#ifndef DEVMODE
# define DEVMODE 0
#endif

size_t const		MAX_SIZE_T = std::numeric_limits<int>::max(); // ~ 2GB
std::string const	SERVER_SOFTWARE = "webserv/1.0";

// config

size_t const	DEFAULT_MAX_CLIENT_BODY_SIZE = 1024 * 1024; // 1MB (safe beacause < INT_MAX)

// http

enum ParseStatus
{
	PARSE_SUCCESS = 200,
	PARSE_ERR_BAD_REQUEST = 400,
	PARSE_ERR_VERSION_NOT_SUPPORTED = 505,
	PARSE_ERR_LENGTH_REQUIRED = 411,
	PARSE_INTERNAL_SERVER_ERROR = 500,
};

#endif
