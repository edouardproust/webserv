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

bool	utils::fileExists(std::string const& path)
{
	if (path.empty() || path[0] != '/')
		return false;
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0);
}

size_t	utils::getFileSize(const std::string& path)
{
	std::ifstream file(path.c_str(), std::ios::binary | std::ios::ate);
	if (!file.is_open())
		return 0;
	return file.tellg();
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

size_t	utils::hexToSizeT(const std::string& hexStr)
{
	if (hexStr.empty())
		return static_cast<size_t>(-1);
	size_t result;
	std::stringstream ss;
	ss << std::hex << hexStr;
	if (!(ss >> result))
		 return static_cast<size_t>(-1);
	return result;
}

char	utils::hexToChar(const std::string& hex)
{
	if (hex.length() != 2)
		return -1;
	int value = 0;
	for (size_t i = 0; i < 2; i++)
	{
		char c = hex[i];
		if (c >= '0' && c <= '9')
			value = value * 16 + (c - '0');
		else if (c >= 'A' && c <= 'F')
			value = value * 16 + (c - 'A' + 10);
		else if (c >= 'a' && c <= 'f')
			value = value * 16 + (c - 'a' + 10);
		else
			return -1;
	}
	return static_cast<char>(value);
}

std::string	utils::readFile(const std::string& path)
{
	std::ifstream file(path.c_str(), std::ios::binary);
	if (!file.is_open())
		return "";
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
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

std::string utils::joinRelativePath(const std::string& path) {
	if (path.empty())
		throw std::runtime_error("empty relative path");

	if (path.rfind("./", 0) != 0)
		throw std::runtime_error("relative path must start with './'");

	char cwd[PATH_MAX];
	if (!getcwd(cwd, sizeof(cwd)))
		throw std::runtime_error("failed to get current working directory to join relative path)");

	std::string rel = path.substr(2); // remove "./"
	std::string fullPath = cwd;

	if (!rel.empty() && rel[0] != '/')
		fullPath += "/";
	fullPath += rel;

	return normalizePath(fullPath);
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

/**
 * Delete leading and trailing space and tab chars from a string.
 */
std::string	utils::trim(const std::string& str)
{
	size_t start = 0;

	while (start < str.size() && (str[start] == ' ' || str[start] == '\t'))
		start++;

	size_t end = str.size();
	while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t'))
		end--;
	return str.substr(start, end - start);
}

/**
 * Remove the domain of a URL.
 *
 * Examples:
 * - `https://mydomain.com/path/file.txt` -> `/path/file.txt`
 * - `anything://mydomain.com/file.txt -> `/file.txt`
 * - `/path/file.txt` -> `/path/file.txt`
 * - `http://mydomain.com` -> empty string
 */
std::string	utils::trimDomain(std::string const& url)
{
	size_t pos = url.find("://");
	size_t start = (pos != std::string::npos) ? url.find('/', pos + 3) : 0;
	if (start == std::string::npos)
		return "/";
	return url.substr(start);
}

/**
 * Truncate a string as an excerpt, up to`n` chars.
 *
 * Will display `... (x bytes total)` after a truncated text.
 */
std::string	utils::excerpt(size_t n, std::string const& str) {
	std::string excerpt = str.substr(0, n);
	if (str.size() > n)
    	excerpt += "... (" + toString(str.size()) + " bytes total)";
	return excerpt;
}
