#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <limits>
#include <cstddef>

// global

#ifndef DEVMODE
# define DEVMODE 0
#endif

size_t const	MAX_SIZE_T = std::numeric_limits<int>::max(); // ~ 2GB
size_t const	MIN_SIZE_T = std::numeric_limits<int>::min(); // ~ -2GB

// config

size_t const	DEFAULT_MAX_CLIENT_BODY_SIZE = 1024 * 1024; // 1MB (safe beacause < INT_MAX)

// http

enum ParseStatus
{
	PARSE_SUCCESS = 200, // OK
	PARSE_ERR_BAD_REQUEST = 400, // Wrong request line format
	PARSE_ERR_HTTP_VERSION_NOT_SUPPORTED = 505, // Not HTTP/1.0 (or HTPP/1.1 for later)
	PARSE_ERR_LENGTH_REQUIRED = 411 ,  // For empty body eg in POST will be used later when body is parsed
	PARSE_ERR_HEADER_SYNTAX_ERROR = 400, // Wrong header line
	PARSE_ERR_HEADER_NAME_EMPTY = 400, // Empty header name
	PARSE_ERR_MISSING_HOST_HEADER = 400 // To implement for HTTP 1.1
};

#endif