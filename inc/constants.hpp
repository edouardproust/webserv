#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <limits>
#include <cstddef>

#ifndef DEVMODE
# define DEVMODE 0
#endif

size_t const	MAX_SIZE_T = std::numeric_limits<int>::max(); // ~ 2GB
size_t const	MIN_SIZE_T = std::numeric_limits<int>::min(); // ~ -2GB

// config

unsigned long const	MAX_CLIENT_BODY_SIZE = 2UL * 1024UL * 1024UL * 1024UL; // 2Go

// static

size_t const	MAX_FILE_SIZE = 10485760; // 10MB

// http

enum ParseStatus
{
	NOT_SET = 0,
	PARSE_SUCCESS = 200,
	PARSE_ERR_BAD_REQUEST = 400,
	PARSE_ERR_HTTP_VERSION_NOT_SUPPORTED = 505,
	PARSE_ERR_LENGTH_REQUIRED = 411
};


#endif
