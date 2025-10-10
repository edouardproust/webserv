#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#ifndef DEVMODE
# define DEVMODE 0
#endif

unsigned long const	MAX_CLIENT_BODY_SIZE = 2UL * 1024UL * 1024UL * 1024UL; // 2Go

enum Method
{
	GET,
	POST,
	DELETE,
	UNKNOWN
};

enum ParseStatus
{
	PARSE_SUCCESS,  //200, OK
	PARSE_ERR_BAD_REQUEST, // 400 Wrong request line formst
	PARSE_ERR_HTTP_VERSION_NOT_SUPPORTED, // 505 Not HTTP/1.0 (or HTPP/1.1 for later)
	PARSE_ERR_LENGTH_REQUIRED,  //411 For empty body eg in POST will be used later when body is parsed
	PARSE_ERR_HEADER_SYNTAX_ERROR, // 400 Wrong header line
	PARSE_ERR_HEADER_NAME_EMPTY, // 400 Empty header name
	PARSE_ERR_MISSING_HOST_HEADER // 400 To implement for HTTP 1.1
};

#endif