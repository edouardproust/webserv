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
size_t const		EXCERPT_LENGTH = 200;

// config

size_t const	DEFAULT_MAX_CLIENT_BODY_SIZE = 1024 * 1024; // 1MB (safe beacause < INT_MAX)

// static

size_t const	MAX_FILE_SIZE = 10 * 1024 * 1024; // 10MB

#endif
