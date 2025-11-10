#include "config/LocationBlock.hpp"
#include "config/Config.hpp"

/**
 * Default location block (path = "/"). Used if not location matches the request URI.
 */
LocationBlock::LocationBlock(ServerBlock const* server)
: _server(server), _path("/"), _return(std::make_pair(-1, "")), _isSetClientMaxBodySize(false), _isDefaultLocation(true)
{
	// PUT method is disabled by default because it requires a upload_store value
	_allowedMethods = Request::getSupportedMethods();
	_allowedMethods.erase("PUT");
}

/**
 * Sets the path and parses the content of a location block.
 *
 * May throw an exception.
 */
LocationBlock::LocationBlock(ServerBlock* server, std::string& path, std::string const& blockContent)
: _server(server), _return(std::make_pair(-1, "")), _isSetClientMaxBodySize(false), _isDefaultLocation(false) {
	_setPath(path);
	_parse(blockContent);
	// Extra checks
	if (isAllowedMethod("PUT") && _uploadStore.empty())
		throw std::runtime_error("Directive upload_store is mandatory for PUT method");
}

LocationBlock::LocationBlock(const LocationBlock &other)
: _server(other._server),
  _path(other._path),
  _autoindex(other._autoindex),
  _allowedMethods(other._allowedMethods),
  _return(other._return),
  _clientMaxBodySize(other._clientMaxBodySize),
  _isSetClientMaxBodySize(other._isSetClientMaxBodySize),
  _uploadStore(other._uploadStore),
  _indexFiles(other._indexFiles),
  _errorPages(other._errorPages),
  _cgi(other._cgi),
  _isDefaultLocation(other._isDefaultLocation)
{}

LocationBlock&	LocationBlock::operator=(LocationBlock const& other) {
	if (this != &other) {
		_server = other._server;
		_path = other._path;
		_autoindex= other._autoindex;
		_allowedMethods= other._allowedMethods;
		_return = other._return;
		_clientMaxBodySize = other._clientMaxBodySize;
		_isSetClientMaxBodySize = other._isSetClientMaxBodySize;
		_indexFiles = other._indexFiles;
		_errorPages = other._errorPages;
		_cgi = other._cgi;
		_isDefaultLocation = other._isDefaultLocation;
	}
	return *this;
}

LocationBlock::~LocationBlock() {}

/**
 * Parses the content of a location block.
 *
 * Throws a runtime_error() exception if:
 * - unsupported directive
 * - directive arguments are invalid
 * - unexpected ';' or '{' found
 * - unclosed quoted string
 * - directive not terminated with ';'
 */
void	LocationBlock::_parse(std::string const& content) {
	std::string token = "";
	Tokens tokens;
	bool inQuotes = false;
	for (size_t i = 0; i < content.size(); ++i) {
		char c = content[i];
		if (c == '#')
			Config::skipComment(content, i);
		else if (c == '{')
			throw std::runtime_error("Unexpected '{'");
		else if (c == ';')
			_parseDirective(token, tokens, inQuotes);
		else if (isspace(c) && !inQuotes)
			Config::addTokenIf(token, tokens);
		else if (c == '"')
			inQuotes = !inQuotes;
		else
			token += c;
	}
	std::string directiveName = !tokens.empty() ? tokens[0] : !token.empty() ? token : "";
	if (directiveName != "")
		throw std::runtime_error(directiveName + ": Directive not terminated with ';'");
    if (inQuotes)
        throw std::runtime_error("Unclosed quoted string");
}

/*
 * Parses a directive inside a location block.
 *
 * `root` directive is not supported in LocationBlock (use server root instead).
 *
 * Throws a runtime_error() exception if:
 * - unexpected ';' found
 * - unclosed quoted string
 * - directive is unsupported or has invalid arguments
 */
void	LocationBlock::_parseDirective(std::string& token, std::vector<std::string>& tokens, bool inQuotes) {
	Config::addTokenIf(token, tokens);
	if (tokens.empty())
		throw std::runtime_error("Unexpected ';'");
	else if (inQuotes)
		throw std::runtime_error("Unclosed quoted string");

	std::string directiveName = tokens[0];
	try {
		if (directiveName == "autoindex")
			_setAutoindex(tokens);
		else if (directiveName == "allowed_methods")
			_setAllowedMethods(tokens);
		else if (directiveName == "return")
			_setReturn(tokens);
		else if (directiveName == "client_max_body_size")
			_setClientMaxBodySize(tokens);
		else if (directiveName == "upload_store")
			_setUploadStore(tokens);
		else if (directiveName == "index")
			_setIndexFiles(tokens);
		else if (directiveName == "cgi")
			_setCgi(tokens);
		else if (directiveName == "error_page")
			_setErrorPages(tokens);
		// -- additional directives can be added here --
		else {
			throw std::runtime_error("Unsupported directive "
				"(supported: autoindex, allowed_methods, return, client_max_body_size, upload_store, index, cgi, error_page)");
				// -- update this list if blocks added --
		}
	} catch (std::exception& e) {
		throw std::runtime_error(directiveName + ": " + e.what()); // wrap error msg with the directive name
	}

	tokens.clear();
}

void	LocationBlock::setServer(ServerBlock* server) {
	_server = server;
}

void	LocationBlock::_setPath(std::string& path) {
	if (!utils::isAbsolutePath(path))
		throw std::runtime_error("Not an absolute path");
	_path = utils::normalizePath(path);
}

/**
 * Enables or disables directory listing for this location.
 *
 * Syntax: `autoindex on|off;`
 *
 * Default is "on".
 *
 * Throws a runtime_error() exception if:
 * - arguments are invalid
 * - value is not "on" or "off"
 */
void	LocationBlock::_setAutoindex(Tokens const& tokens) {
	if (tokens.size() != 2)
		throw std::runtime_error("Should have 1 argument");
	std::string const& autoindex = tokens[1];
	if (autoindex.empty())
		throw std::runtime_error("Value is an empty string");
	if (autoindex != "on" && autoindex != "off")
		throw std::runtime_error("Value should be \"on\" or \"off\"");
	_autoindex = autoindex;
}

/**
 * Defines allowed HTTP methods in this location.
 *
 * Syntax: `allowed_methods method1 [method2 ...];`
 *
 * If not set, all methods are allowed.
 *
 * May throw a runtime_error() exception if:
 * - no method is given
 * - a method is an empty string or is not supported
 */
void LocationBlock::_setAllowedMethods(Tokens const& tokens) {
	if (tokens.size() < 2)
		throw std::runtime_error("Should have at least 1 method");
	_allowedMethods.clear(); // renitialise before filling up
	for (size_t i = 1; i < tokens.size(); ++i) {
		const std::string& method = tokens[i];
		if (method.empty())
			throw std::runtime_error("Method is an empty string");
		if (!Request::isSupportedMethod(method))
			throw std::runtime_error("Not supported method: " + method);
		_allowedMethods.insert(method);
	}
}

/**
 * Sets up HTTP redirection or simple return code for this location.
 *
 * Syntax: `return code [target];`
 * `target` is required for 3xx codes.
 * Only one return directive allowed.
 *
 * Throws a runtime_error() exception if:
 * - arguments are invalid
 * - code is out of range 100-599
 * - target is missing for redirection codes (300-399)
 * - code conflicts with an error_page defined in the parent server block
 */
void LocationBlock::_setReturn(Tokens const& tokens) {
	if (_return.first != -1)
			throw std::runtime_error("Duplicate directive");
	if (tokens.size() < 2 || tokens.size() > 3)
			throw std::runtime_error("Should have 1 or 2 arguments");
	int code = utils::toSizeT(tokens[1]); // throw exception if empty, etc.
	std::string target;
	if (tokens.size() == 3) {
		if (tokens[2].empty())
			throw std::runtime_error("Target is an empty string");
		target = tokens[2];
	}
	if (code < 100 || code > 599)
		throw std::runtime_error("Code " + utils::toString(code) + " is out of range 100-599");
	if (code >= 300 && code < 400 && target.empty())
		throw std::runtime_error("Target missing for redirection " + utils::toString(code));
	if (_server->getErrorPages().find(code) != _server->getErrorPages().end())
        throw std::runtime_error("Return '" + utils::toString(code) + "' conflicts with error_page in server block");
	_return.first = code;
	_return.second = target;
}

/**
 * Overrides server's client max body size limit for this location.
 *
 * Syntax: `client_max_body_size size;`
 * Examples: `client_max_body_size 1024;`, `client_max_body_size 2M;`
 * Size is the number of bytes followed by optional unit (B, K, M, G).
 *
 * Server's limit is used if not set in this location.
 *
 * Exception is thrown if:
 * - arguments are invalid
 * - size is 0, has a wrong syntax or overflows size_t
 */
void	LocationBlock::_setClientMaxBodySize(Tokens const& tokens) {
	if (tokens.size() != 2)
		throw std::runtime_error("Format must be \"client_max_body_size size;\"");
	size_t result = Config::parseSize(tokens[1]); // throw exception if empty, wrong syntax or overflow
	if (result == 0)
		throw std::runtime_error("Must be more than 0 bytes:" + tokens[1]); // Size cannot be 0
    _clientMaxBodySize = result;
    _isSetClientMaxBodySize = true;
}

void	LocationBlock::_setUploadStore(Tokens const& tokens) {
	if (tokens.size() != 2)
		throw std::runtime_error("Format must be \"upload_store path;\"");
	std::string path = tokens[1];
	if (path.empty())
		throw std::runtime_error("Path is an empty string");
	std::string const& root = getRoot();
	if (utils::isRelativePath(path) && root.empty())
		throw std::runtime_error("A relative path requires a server root");
	if (utils::isRelativePath(path))
		path = utils::pathsJoin(root, path);
	_uploadStore = path;
}

/**
 * Overrides server's custom error pages for this location.
 *
 * Syntax: `error_page code1 [code2 ...] path;`
 * Error page path can be an absolute, or relative to the server root.
 * A server root is required.
 *
 * Defaults to server error pages if not set in this location. If none set in server either, built-in error pages are used.
 *
 * Exception is thrown if:
 * - arguments are invalid
 * - No server root is set
 * - the path is an empty string
 * - an HTTP code is out of range (300-599)
 */
void	LocationBlock::_setErrorPages(Tokens const& tokens) {
	if (tokens.size() < 3)
		throw std::runtime_error("Format must be \"error_page code1 [code2 ...] path;\"");
	// Check path (last argument)
	std::string path = tokens.back();
	if (path.empty())
		throw std::runtime_error("Path is an empty string");
	std::string const& root = getRoot();
	if (utils::isRelativePath(path) && root.empty())
		throw std::runtime_error("A relative path requires a server root");
	if (utils::isRelativePath(path))
		path = utils::pathsJoin(root, path);
	for (size_t j = 1; j < tokens.size() - 1; ++j) {
		// Check each HTTP status codes
		std::string codeStr = tokens[j];
		size_t code = utils::toSizeT(codeStr); // throw if empty string, not an number or size_t overflow
		if (code < 300 || code > 599)
			throw std::out_of_range("Invalid HTTP code: " + codeStr);
		// Add to map (overrides value of existing codes)
		_errorPages[code] = utils::normalizePath(path);
	}
}

/**
 * Overrides server's index files for this location. Index files are served if a directory is requested.
 *
 * Syntax: `index file1 [file2 ...];`
 * Index path can be absolute, or relative to server's root;
 * A server root is required.
 *
 * Defaults to server index files if not set in this location. If none set in server either, default index files are used.
 * If no index file is set for the requested directory and autoindex is on, a built-in directory index is served instead.
 *
 * Exception is thrown if:
 * - arguments are invalid
 * - No server root is set
 * - an index file is an empty string
 * - duplicate index files
 */
void	LocationBlock::_setIndexFiles(Tokens const& tokens) {
	if (tokens.size() < 2)
		throw std::runtime_error("Format must be \"index file1 [file2 ...];\"");
	for (size_t j = 1; j < tokens.size(); ++j) {
		std::string path = tokens[j];
		if (path.empty())
			throw std::runtime_error("An index value is an empty string");
		if (utils::isRelativePath(path) && getRoot().empty())
			throw std::runtime_error("A relative path requires a server root");
		_indexFiles.push_back(path);
	}
	if (!utils::hasVectorUniqEntries(_indexFiles))
		throw std::runtime_error("Duplicated index files");
}

/**
 * Configures CGI executables for specific file extensions.
 *
 * Syntax: `cgi extension executable_path;`
 *
 * Exception is thrown if:
 * - arguments are invalid
 * - the executable path is an empty string or a relative path or not accessible on host
 * - the extension is an empty string, does not start with a dot or is already configured in this location
 */
void	LocationBlock::_setCgi(Tokens const& tokens) {
	if (tokens.size() != 3)
		throw std::runtime_error("Format must be \"cgi extension executable_path;\"");
	std::string extension = tokens[1], executable = tokens[2];
	if (extension.empty())
		throw std::runtime_error("Extension is an empty string");
	if (executable.empty())
		throw std::runtime_error("Executable is an empty string");
	if (utils::isRelativePath(executable))
		throw std::runtime_error("Executable path must be an absolute path: " + executable);
    if (extension[0] != '.')
		throw std::runtime_error("Extension must start with a dot: " + extension);
    if (_cgi.find(extension) != _cgi.end())
		throw std::runtime_error("Duplicate extension: " + extension);
	if (!utils::isExecutableFile(executable))
		throw std::runtime_error("Executable cannot be accessed on host: " + executable
			+ "\nPlease review the path or install the missing dependencies");
	_cgi[extension] = executable;
}

ServerBlock const*	LocationBlock::getServer() const {
	return _server;
}

std::string const&	LocationBlock::getPath() const {
	return _path;
}

/**
 * `root` is not supported inside location block. Location inherits of server's `root`.
 * Setting a root for the server is mandatory because `webserv` is a portable program.
 */
std::string const	LocationBlock::getRoot() const {
	return _server->getRoot();
}

/**
 * Defaults to "on" if not set in this location.
 */
std::string const	LocationBlock::getAutoindex() const {
	if (_autoindex.empty())
		return "on";
	return _autoindex;
}

std::set<std::string> const&	LocationBlock::getAllowedMethods() const {
	return _allowedMethods;
}

std::pair<int, std::string>	const&	LocationBlock::getReturn() const {
	return _return;
}

/**
 * Defaults to server's `client_max_body_size` if none set in this location.
 */
size_t	LocationBlock::getClientMaxBodySize() const {
	if (!_isSetClientMaxBodySize && _server)
		return _server->getClientMaxBodySize();
	return _clientMaxBodySize;
}

bool	LocationBlock::getClientMaxBodySizeSet() const {
	return _isSetClientMaxBodySize;
}

std::string const&	LocationBlock::getUploadStore() const {
	return _uploadStore;
}

CgiDirective const&	LocationBlock::getCgi() const {
	return _cgi;
}

std::vector<std::string> const&	LocationBlock::getIndexFiles() const {
	if (_indexFiles.empty() && _server)
		return _server->getIndexFiles();
	return _indexFiles;
}

ErrorPages const&	LocationBlock::getErrorPages() const {
	if (_errorPages.empty() && _server)
		return _server->getErrorPages();
	return _errorPages;
}

std::string const	LocationBlock::getCgiExecutor(std::string const& extension) const {
	if (extension.empty())
		return "";
	CgiDirective::const_iterator it = _cgi.find(extension);
	if (it != _cgi.end())
		return it->second;
	return "";
}

/**
 * `server` pointer should not be `NULL` (this may lead to unexpected behaviour).
 */
LocationBlock const&	LocationBlock::getDefaultLocation(ServerBlock const* server) {
	static LocationBlock defaultLocation(server);
    return defaultLocation;
}

/**
 * For a given extension, tells if this location allows dynamic resolution of a request (CGI).
 */
bool	LocationBlock::isCgi(std::string const& extension) const {
	if (extension.empty())
		return false;
	for (CgiDirective::const_iterator it = _cgi.begin(); it != _cgi.end(); ++it) {
		if (it->first == extension)
			return true;
	}
	return false;
}

/**
 * Tells if this location corresponds to a redirection.
 */
bool	LocationBlock::isRedirection() const {
	return _return.first != -1 && !_return.second.empty();
}

/**
 * Tells if this location allows a given method.
 */
bool	LocationBlock::isAllowedMethod(std::string const& method) const {
	return _allowedMethods.empty() || (_allowedMethods.find(method) != _allowedMethods.end());
}

bool	LocationBlock::isAllowedClientBodySize(size_t size, std::string const& method) const {
	static std::string const methodsWithBody[] = {"POST", "PUT", "PATCH"};
    if (utils::isInArray(method, methodsWithBody)) {
        return size <= getClientMaxBodySize();
	}
    return true; // GET, HEAD, DELETE, etc.
}

/**
 * Tells if this location has been built using the default constructor.
 */
bool	LocationBlock::isDefaultLocation() const {
	return _isDefaultLocation;
}

std::ostream&	operator<<(std::ostream& os, LocationBlock const& rhs) {
	std::string const in = "  "; // indentation
	os << in << "- Default location: " << (rhs.isDefaultLocation() ? "yes" : "no") << "\n";
	os << in << "- path: " << PrintableString(rhs.getPath()) << "\n";
	os << in << "- root: " << PrintableString(rhs.getRoot()) << "\n";
	os << in << "- autoindex: " << PrintableString(rhs.getAutoindex()) << "\n";

	std::set<std::string> const& _allowedMethods = rhs.getAllowedMethods();
	os << in << "- allowed_methods: " << _allowedMethods.size() << "\n";
	for (std::set<std::string>::const_iterator it = _allowedMethods.begin(); it != _allowedMethods.end(); ++it)
		os << in << "  - " << *it << "\n";

	std::pair<int, std::string> const& ret = rhs.getReturn();
	os << in << "- return: " << (ret.first != -1
		? utils::toString(ret.first) + " -> " + PrintableString(ret.second)
		: "[empty]") << "\n";

	os << in << "- client_max_body_size: " << PrintableString(rhs.getClientMaxBodySizeSet()
		? utils::toString(rhs.getClientMaxBodySize()) : "") << "\n";

	os << "- upload_store: " << PrintableString(rhs.getUploadStore()) << "\n";

	std::vector<std::string> const& indexFiles = rhs.getIndexFiles();
	os << in << "- index_files: " << indexFiles.size() << "\n";
	for (size_t i = 0; i < indexFiles.size(); ++i)
		os << in << "  - " << indexFiles[i] << "\n";

	ErrorPages const& errorPages = rhs.getErrorPages();
	os << in << "- error_page: " << errorPages.size() << "\n";
	for (ErrorPages::const_iterator it = errorPages.begin(); it != errorPages.end(); ++it)
		os << in << "  - " << it->first << " -> " << it->second << "\n";

	CgiDirective const& cgi = rhs.getCgi();
	os << in << "- cgi: " << cgi.size() << "\n";
	for (CgiDirective::const_iterator it = cgi.begin(); it != cgi.end(); it++) {
		os << in << "  - " << it->first << " -> " << it->second << "\n";
	}

	return os;
}
