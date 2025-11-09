#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <limits>
#include <string>
#include <cstddef>

// global

#ifndef DEVMODE
# define DEVMODE 0
#endif

std::string const	SERVER_NAME = "webserv";
std::string const	SERVER_VERSION = "1.0";
std::string const	SERVER_REPO = "https://github.com/edouardproust/webserv";
size_t const		EXCERPT_LENGTH = 500;

size_t const		MAX_SIZE_T = std::numeric_limits<int>::max(); // ~ 2GB
std::string const	SERVER_SOFTWARE = SERVER_NAME + "/" + SERVER_VERSION;

// network

#define FT_DEFAULT_CLIENT_BUFFER_SIZE 1024
#define FT_MAX_EVENT_SIZE 100

// parser

size_t const		HEADER_MAX_SIZE = 8192;

// config

size_t const	DEFAULT_MAX_CLIENT_BODY_SIZE = 1024 * 1024; // 1MB (safe beacause < INT_MAX)
size_t const	DEFAULT_CGI_TIMEOUT = 5; // seconds

// static

size_t const	MAX_FILE_SIZE = 10 * 1024 * 1024; // 10MB

#endif
