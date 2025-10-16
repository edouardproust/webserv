#include "utils/utils.hpp"
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <climits>

bool	utils::isInt(std::string const& str)
{
	if (str.empty())
		return false;
	char* endptr = NULL;
	errno = 0;
	long value = std::strtol(str.c_str(), &endptr, 10);
	if (*endptr != '\0')
		return false;
	if ((errno == ERANGE) || (value > INT_MAX) || (value < INT_MIN))
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

unsigned long	utils::parseSize(std::string const& value)
{
	if (value.empty())
		return 0;
	char unit = value[value.size() - 1];
	unsigned long multiplier = 1;
	static const std::string units = "KkMmGg";
	std::string numberStr =
		(units.find(unit) != std::string::npos)
			? value.substr(0, value.size() - 1)
			: value;
	if (!isInt(numberStr)) {
		throw std::runtime_error("Invalid size value: " + value);
	}
	unsigned long number = std::strtoul(numberStr.c_str(), NULL, 10);
	if (unit == 'K' || unit == 'k') {
		multiplier = 1024;
	} else if (unit == 'M' || unit == 'm') {
		multiplier = 1024 * 1024;
	} else if (unit == 'G' || unit == 'g') {
		multiplier = 1024 * 1024 * 1024;
	}
	return number * multiplier;
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
