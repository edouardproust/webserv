#include "utils/Const.hpp"

size_t const		Const::MAX_SIZE_T = std::numeric_limits<int>::max(); // ~ 2GB

std::string const	Const::SERVER_NAME = "webserv";
std::string const	Const::SERVER_VERSION = "1.0";
std::string const	Const::SERVER_REPO = "https://github.com/edouardproust/webserv";
std::string const	Const::SERVER_SOFTWARE = SERVER_NAME + "/" + SERVER_VERSION;
