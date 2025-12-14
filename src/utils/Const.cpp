#include "utils/Const.hpp"

std::string const	Const::DEFAULT_CONFIG_FILE_PATH = "config/default.config";
size_t const		Const::MAX_SIZE_T = std::numeric_limits<int>::max();		// ~ 2GB
size_t const		Const::ABSOLUTE_MAX_CLIENT_BODY_SIZE = 200 * 1024 * 1024;	// 200 MB (required by ubuntu_tester)
size_t const		Const::READ_WRITE_LOG_THRESHOLD_SIZE = 1024 * 1024;			// 1MB (to prevent write/read log spam)

std::string const	Const::SERVER_NAME = "webserv";
std::string const	Const::SERVER_VERSION = "1.0";
std::string const	Const::SERVER_REPO = "https://github.com/edouardproust/webserv";
std::string const	Const::SERVER_SOFTWARE = SERVER_NAME + "/" + SERVER_VERSION;
