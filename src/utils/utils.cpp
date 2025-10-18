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
	if (*endptr != '\0' || errno == ERANGE
		|| value > static_cast<long>(MAX_SIZE_T) || value < static_cast<long>(MIN_SIZE_T))
		return false;
	return true;
}

bool	utils::isAccessibleDirectory(std::string const& path) {
	// not empty and starts with '/'
	if (path.empty() || path[0] != '/')
		return false;
	// check directory file type
	struct stat fileinfo;
	if (stat(path.c_str(), &fileinfo) == -1)
		return false;
	if (!S_ISDIR(fileinfo.st_mode))
		return false;
	// is accessible ?
	if (access(path.c_str(), R_OK | X_OK) != 0)
    	return false;
	return true;
}

bool	utils::isExecutableFile(std::string const& path) {
	// not empty and starts with '/'
	if (path.empty() || path[0] != '/')
		return false;
	struct stat fileinfo;
	if (stat(path.c_str(), &fileinfo) == -1)
		return false;
	// check regular file type
	if (!S_ISREG(fileinfo.st_mode))
		return false;
	// check execute permission
	if (access(path.c_str(), X_OK) != 0)
		return false;
	return true;
}

/**
 * Only checking for Linux distros (path starting by '/')
 */
bool	utils::isAbsolutePath(std::string const& path) {
	if (path.empty() || path[0] != '/')
		return false;
    return true;
}

/**
 * Convert a string into an unsigned int only if the string
 * contains digits or start with + or -.
 *
 * Throw a runtime_error exception if empty str, invalid number or int overflow
 */
size_t	utils::toSizeT(std::string const& str)
{
	if (str.empty())
		throw std::runtime_error("Numeric value is an empty string");
	char* endptr = NULL;
	errno = 0;
	long value = std::strtol(str.c_str(), &endptr, 10);
	if (*endptr != '\0')
		throw std::runtime_error("Invalid numeric value: " + str);
	if (errno == ERANGE || value > static_cast<long>(MAX_SIZE_T)
		|| value < static_cast<long>(MIN_SIZE_T))
		throw std::runtime_error("Numeric value out of range: " + str);
	return static_cast<size_t>(value);
}

std::string& utils::normalizePath(std::string& path) {
	if (path.size() > 1 && path[path.size() - 1] == '/')
		path.erase(path.size() - 1);
	return path;
}

/**
 * Returns empty string "" in cse of failure.
 */
std::string	utils::getFileExtension(std::string const& path) {
	size_t dotPos = path.rfind('.');
	if (dotPos == std::string::npos || dotPos == path.length() - 1)
		return "";
	return path.substr(dotPos);
}

std::string	utils::toLowerCase(const std::string& str)
{
	std::string normalized = str;
	for (size_t i = 0; i < normalized.length(); ++i)
		normalized[i] = std::tolower(normalized[i]);
	return normalized;
}
