#include "utils/utils.hpp"

bool	utils::isInt(std::string const& str)
{
	if (str.empty())
		return false;
	char* endptr = NULL;
	errno = 0;
	long value = std::strtol(str.c_str(), &endptr, 10);
	if (*endptr != '\0' || errno == ERANGE || value > static_cast<long>(MAX_SIZE_T) || value < 0)
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

static bool	_checkFileTypeAndAccess(std::string const& path, mode_t expectedType, int accessMode) {
    if (path.empty() || path[0] != '/')
        return false;
	// is existing path?
    struct stat st;
    if (stat(path.c_str(), &st) == -1)
        return false;
	// is expected type?
    if ((st.st_mode & S_IFMT) != expectedType)
        return false;
	// is accessible?
    if (access(path.c_str(), accessMode) != 0)
        return false;
    return true;
}

bool	utils::isAccessibleDirectory(std::string const& path) {
    return _checkFileTypeAndAccess(path, S_IFDIR, R_OK | X_OK);
}

bool	utils::isReadableFile(std::string const& path) {
    return _checkFileTypeAndAccess(path, S_IFREG, R_OK);
}

bool	utils::isExecutableFile(std::string const& path) {
    return _checkFileTypeAndAccess(path, S_IFREG, X_OK);
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
	if (errno == ERANGE || value > static_cast<long>(MAX_SIZE_T) || value < 0)
		throw std::runtime_error("Numeric value out of range: " + str);
	return static_cast<size_t>(value);
}

/**
 * Returns the file extension, including the dot
 *
 * - path "/whatever/test.php" -> returns ".php"
 * - path "https://mydomaine/index.py" -> returns ".py"
 * - path "/test" or "/test." -> returns "" (empty string)
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

/**
 * Split a string into substrings separated by a given delimiter.
 *
 * Consecutive delimiters or leading/trailing delimiters are ignored,
 * so empty substrings are skipped.
 *
 * Example: `split("a//b/c/", '/')` -> `{"a", "b", "c"}`
 */
std::vector<std::string>	utils::split(std::string const& s, char delim) {
	std::vector<std::string> elems;
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim))
		if (!item.empty()) elems.push_back(item);
	return elems;
}

/**
 * Merge two strings into a valid path.
 *
 * Trailing slashes are not trimmed.
 *
 * Examples:
 * `joinPath("", "mydomain/index.htm")` -> `/mydomain/index.htm`
 * `joinPath("var/www/html/", "")` -> `/var/www/html/`
 * `joinPath("/var/www/html/", "/index.htm")` -> `/var/www/html/index.htm`
 * `joinPath("/var/www/html", "test/")` -> `/var/www/html/test/`
 * `joinPath("", "")` -> `/`
 */
std::string utils::joinPath(std::string const& lhs, std::string const& rhs) {
	if (lhs.empty()) {
		if (rhs.empty()) return "/";
		return (!rhs.empty() && rhs[0] == '/') ? rhs : "/" + rhs;
	}
	std::string joinedPath = lhs;
	if (!joinedPath.empty() && joinedPath[joinedPath.size() - 1] != '/')
		joinedPath += '/';
	if (!rhs.empty() && rhs[0] == '/')
		joinedPath += rhs.substr(1);
	else
		joinedPath += rhs;
	return joinedPath;
}

/**
 * Normalize a path by:
 * - Removing redundant '.' segments
 * - Resolving '..' segments
 * - Collapsing multiple consecutive '/' into a single '/'
 * - Ensuring the path starts with a single '/'
 * - Removing trailing '/' (except for the root '/')
 *
 * Examples:
 * `_normalizePath("/var/www/html/.././index.html")` -> `/var/www/index.html`
 * `_normalizePath("//var///www////html/test//")` -> `/var/www/html/test`
 * `_normalizePath("/./././")` -> `/`
 * `_normalizePath("/a/b/../../c/")` -> `/c`
 */
std::string utils::normalizePath(std::string const& path) {
	std::vector<std::string> parts = utils::split(path, '/');
	std::vector<std::string> clean;
	for (size_t i = 0; i < parts.size(); ++i) {
		if (parts[i] == "..") {
			if (!clean.empty()) clean.pop_back();
		} else if (parts[i] != "." && !parts[i].empty()) {
			clean.push_back(parts[i]);
		}
	}
	std::string normalized = "/";
	for (size_t i = 0; i < clean.size(); ++i) {
		normalized += clean[i];
		if (i + 1 < clean.size()) normalized += "/";
	}
	return normalized;
}

std::string	utils::trimDomain(std::string const& url)
{
	size_t pos = url.find("://");
	size_t start = (pos != std::string::npos) ? url.find('/', pos + 3) : 0;
	if (start == std::string::npos)
		return "/";
	return url.substr(start);
}
