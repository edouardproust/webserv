#include "utils/utils.hpp"

bool	utils::isInt(std::string const& str)
{
	if (str.empty())
		return false;
	char* endptr = NULL;
	errno = 0;
	long value = std::strtol(str.c_str(), &endptr, 10);
	if (*endptr != '\0' || errno == ERANGE || value > static_cast<long>(Const::MAX_SIZE_T) || value < 0)
		return false;
	return true;
}

/**
 * Checks if the path starts by "/" and is not empty.
 *
 * Only checking for Linux distros (path starting by '/')
 */
bool	utils::isAbsolutePath(std::string const& path) {
	return (!path.empty() && path[0] == '/');
}

/**
 * Checks if the path starts by "./" and is not empty.
 */
bool	utils::isRelativePath(std::string const& path) {
	return (!path.empty() && path[0] != '/');
}

/**
 * Path needs to be an absolute path even if stat and access can take relative paths as param.
 * This prevents unexpected behaviour as it forces the config parser to resolve all paths into absolute
 * before running this function.
 */
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

bool	utils::isWritableDirectory(const std::string& path) {
	return _checkFileTypeAndAccess(path, S_IFDIR, W_OK | X_OK);
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
	if (errno == ERANGE || value > static_cast<long>(Const::MAX_SIZE_T) || value < 0)
		throw std::runtime_error("Numeric value out of range: " + str);
	return static_cast<size_t>(value);
}

size_t	utils::hexToSizeT(std::string const& hexStr)
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

char	utils::hexToChar(std::string const& hex)
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

std::string	utils::toUpper(std::string const& s)
{
	std::string up = s;
	for (std::string::size_type i = 0; i < s.size(); ++i) {
		up[i] = std::toupper(static_cast<unsigned char>(s[i]));
	}
	return up;
}

std::string	utils::readFile(std::string const& path)
{
	std::ifstream file(path.c_str(), std::ios::binary);
	if (!file.is_open())
		return "";
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

std::string	utils::getFileName(const std::string& path)
{
	size_t lastSlash = path.find_last_of('/');
	if (lastSlash != std::string::npos)
		return path.substr(lastSlash + 1);
	return path;
}

/**
 * Returns the file extension, including the dot
 *
 * - path "/whatever/test.php" -> returns ".php"
 * - path "https://mydomaine/index.py" -> returns ".py"
 * - path "/test" or "/test." -> returns "" (empty string)
 */
std::string	utils::getFileExtension(std::string const& path)
{
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
std::vector<std::string>	utils::split(std::string const& s, char delim)
{
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
 * Examples:
 * `pathsJoin("", "mydomain/index.htm")` -> `/mydomain/index.htm`
 * `pathsJoin("var/www/html/", "")` -> `/var/www/html/`
 * `pathsJoin("/var/www/html/", "/index.htm")` -> `/var/www/html/index.htm`
 * `pathsJoin("/var/www/html", "test/")` -> `/var/www/html/test/`
 * `pathsJoin("", "")` -> `/`
 *
 * Trailing slashes are not trimmed.
 * Usage of normalizePath() resolves any `.` or `..` in the path.
 */
std::string utils::pathsJoin(std::string const& lhs, std::string const& rhs)
{
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
	return normalizePath(joinedPath);
}

std::string utils::securedPathsJoin(std::string const& rootPath, std::string const& otherPath)
{
	std::string joinedNormalizedPath = pathsJoin(rootPath, otherPath);
	// Joined path should be included in `lhs` (path traversal check)
	std::string normalizedRoot = normalizePath(rootPath);
    	if (normalizedRoot.empty()) normalizedRoot = "/";
	if (joinedNormalizedPath.rfind(normalizedRoot, 0) != 0
		|| (joinedNormalizedPath.size() > normalizedRoot.size()
			&& joinedNormalizedPath[normalizedRoot.size()] != '/'))
		joinedNormalizedPath = ""; // outside of root
	return joinedNormalizedPath;
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
 * `_normalizePath("")` -> (empty string)
 */
std::string utils::normalizePath(std::string const& path)
{
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
std::string	utils::trim(std::string const& str)
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
 * Format time in a given format (UTC).
 *
 * Takes a `time_t` argument as time. Use `time(0)` for the the current time (now).
 */
std::string utils::formatDate(time_t time, const std::string& format)
{
    struct tm* timeinfo = localtime(&time);
    char buffer[64];
    strftime(buffer, sizeof(buffer), format.c_str(), timeinfo);
    return buffer;
}

/**
 * Extract hostname from request headers (at this point we 100% have a host header)
 * - Looks for "host" header (should have been normalized already)
 * - Removes port if present: "example.com:8080" -> "example.com"
 */
std::string	utils::extractHostname(const std::map<std::string, std::string>& headers)
{
	const std::string& host_value = headers.at("host");
	size_t colon_pos = host_value.find(':');
	return (colon_pos != std::string::npos) ? host_value.substr(0, colon_pos) : host_value;
}
