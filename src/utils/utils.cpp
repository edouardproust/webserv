#include "utils/utils.hpp"
#include "constants.hpp"
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>

bool	utils::isInt(std::string const& str)
{
	if (str.empty())
		return false;
	char* endptr = NULL;
	errno = 0;
	long value = std::strtol(str.c_str(), &endptr, 10);
	if (*endptr != '\0' || errno == ERANGE || value > MAX_SIZE_T || value < MIN_SIZE_T)
		return false;
	return true;
}

bool	utils::isAccessibleDirectory(std::string const& path) {
	// is not empty and starts with '/' ?
	if (path.empty() || path[0] != '/')
		return false;
	// is a directory ?
	struct stat fileinfo;
	if (stat(path.c_str(), &fileinfo) == -1)
		return false;
	if ((fileinfo.st_mode & S_IFDIR) == 0)
		return false;
	// is accessible ?
	if (access(path.c_str(), R_OK | X_OK) != 0)
    	return false;
	return true;
}

bool	utils::isAbsolutePath(std::string const& path) {
	if (path.empty() || path[0] != '/')
		return false;
    return true;
}

/**
 * Convert a string into an int only if the string
 * contains digits or start with + or -.
 *
 * May throw an runtime_error() exception in case of int overflow
 */
size_t	utils::toSizeT(std::string const& str)
{
	char* endptr = NULL;
	errno = 0;
	long value = std::strtol(str.c_str(), &endptr, 10);
	if (*endptr != '\0')
		throw std::runtime_error("Invalid numeric value: " + str);
	if (errno == ERANGE || value > MAX_SIZE_T || value < MIN_SIZE_T)
		throw std::runtime_error("Numeric value out of range: " + str);
	return static_cast<size_t>(value);
}

std::string& utils::normalizePath(std::string& path) {
	if (path.size() > 1 && path[path.size() - 1] == '/')
		path.erase(path.size() - 1);
	return path;
}

std::string	utils::getFileExtension(std::string const& path) {
	size_t dotPos = path.rfind('.');
	if (dotPos == std::string::npos || dotPos == path.length() - 1)
		return "";
	return path.substr(dotPos);
}
