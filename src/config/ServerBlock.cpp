#include "config/ServerBlock.hpp"
#include "config/Config.hpp"
#include "colors.hpp"

ServerBlock::ServerBlock(): _isSetClientBodySize(false) {}

/**
 * Parse the content of a server block and sets default index files and listening port `0.0.0.0:80`
 * if empty after parsing.
 * If no location block is defined, a default location is created (path "/" and PUT method forbidden)
 *
 * Parsing may throw an exception.
 */
ServerBlock::ServerBlock(std::string const& blockContent): _isSetClientBodySize(false) {
	_parse(blockContent); // throw
	_setDefaultIndexFiles();
	if (_listen.empty())
      	_listen.insert(HostPortPair("0.0.0.0:80"));
	if (_locations.empty()) {
		_locations.push_back(LocationBlock(this));
		std::cout << FT_WARNING << "Config: server: No location block defined, a default one was created. PUT method is no allowed in this location." << RESET_COLOR << std::endl;
	}
}

ServerBlock::ServerBlock(const ServerBlock &other): _isSetClientBodySize(false) {
	*this = other;
}

ServerBlock& ServerBlock::operator=(ServerBlock const& other) {
    if (this != &other) {
        _root = other._root;
        _listen = other._listen;
        _clientMaxBodySize = other._clientMaxBodySize;
		_uploadStore = other._uploadStore;
        _errorPages = other._errorPages;
        _indexFiles = other._indexFiles;
		// locations
		_locations.clear();
        _locations.reserve(other._locations.size());
        for (size_t i = 0; i < other._locations.size(); ++i) {
            _locations.push_back(other._locations[i]);
            _locations.back().setServer(this);
        }
    }
    return *this;
}

ServerBlock::~ServerBlock() {}

/**
 * Parses the content of a server block.
 *
 * Throws a runtime_error() exception if:
 * - `root` directive is not set or is invalid
 * - unsupported directive or with invalid arguments
 * - unexpected ';' or '{' found
 * - unclosed quoted string or directive not terminated with ';'
 */
void	ServerBlock::_parse(std::string const& content) {
	std::string token = "";
	Tokens tokens;
	bool inQuotes = false;
	int braceDepth = 0;
	for (size_t i = 0; i < content.size(); ++i) {
		char c = content[i];
		if (c == '#')
			Config::skipComment(content, i);
		else if (c == '{')
			_parseBlock(tokens, content, i, braceDepth, inQuotes);
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

/**
 * Parses a block inside a server block.
 *
 * A block is is delimited by '{' and '}' and can contains directives.
 * For now only `location` blocks are supported inside a server block.
 * There can be serveral `location` blocks inside a `server` block.
 *
 * May throw a std::runtime_error() exception.
 */
void	ServerBlock::_parseBlock(Tokens& tokens, std::string const& content, size_t& i, int& braceDepth, bool inQuotes) {
	if (tokens.empty())
		throw std::runtime_error("Unexpected '{'");
	else if (inQuotes)
		throw std::runtime_error("Unexpected '{' in quoted string");
	++braceDepth; // skip '{'
	std::string blockName = tokens[0];
	try {
		if (blockName == "location")
			_addLocation(tokens, content, i, braceDepth);
		// -- additional blocks can be added here --
		else {
			throw std::runtime_error("Unsupported block "
				"(supported: location)");
				// -- update this list if blocks added --
		}
	} catch (std::exception& e) {
		throw std::runtime_error(blockName + (blockName == "location" && tokens.size() > 1 ? " \"" + tokens[1] + "\"" : "") + ": " + e.what()); // wrap error msg with the block name
	}
	tokens.clear();
}

/**
 * Parses a directive inside a server block.
 *
 * A directive is a setting that ends with a ';'.
 * `root` directive is mandatory. Other directives are optionnal and have default values.
 *
 * Throws a runtime_error() exception if:
 * - unexpected ';' found or unclosed quoted string
 * - directive is unsupported or has invalid arguments
 */
void	ServerBlock::_parseDirective(std::string& token, Tokens& tokens, bool inQuotes) {
	Config::addTokenIf(token, tokens);
	if (tokens.empty())
		throw std::runtime_error("Unexpected ';'");
	else if (inQuotes)
		throw std::runtime_error("Unclosed quoted string in server block");

	std::string directiveName = tokens[0];
	try {
		if (directiveName == "root")
			_setRoot(tokens);
		else if (directiveName == "listen")
			_setListen(tokens);
		else if (directiveName == "client_max_body_size")
			_setClientMaxBodySize(tokens);
		else if (directiveName == "error_page")
			_setErrorPages(tokens);
		else if (directiveName == "index")
			_setIndexFiles(tokens);
		else if (directiveName == "upload_store")
			_setUploadStore(tokens);
		// -- additional directives can be added here --
		else {
			throw std::runtime_error(directiveName + ": Unsupported directive.\n"
				"Supported list: listen, root, upload_store, index, error_page, client_max_body_size");
				// -- update this list if directives added --
		}
	} catch (std::exception& e) {
		throw std::runtime_error(directiveName + ": " + e.what()); // wrap error msg with the directive name
	}

	tokens.clear();
}

/**
 * Adds a location block to this server.
 *
 * Syntax: `location path { [directive1 param; ...] }`
 *
 * Throw a std::runtime_error() exception if:
 * - the location does not have a path
 * - this location path is already used by another location
 * - the parsing of the location block throws an exception
 */
void	ServerBlock::_addLocation(Tokens const& tokens, std::string const& content, size_t& i, int& braceDepth) {
	if (tokens.size() != 2)
		throw std::runtime_error("Format must be \"location path { [directive1 param; ...] }\"");

	std::string path = tokens[1];
	// Check if the path does not already exists in another LocationBlock
	for (size_t j = 0; j < _locations.size(); ++j) {
		if (_locations[j].getPath() == path)
			throw std::runtime_error("Path already set in another location: " + path);
	}
	std::string blockContent = Config::getBlockContent(content, i, braceDepth);
	LocationBlock lb(this, path, blockContent); // throw
	_locations.push_back(lb);
}

/**
 * Sets default index files if server root is set and no index directive was found yet.
 */
void	ServerBlock::_setDefaultIndexFiles() {
	if (!_indexFiles.empty())
		return; // do not override if index files are already set
	if (!_root.empty()) { // only set default index files if server root is set (it will always be an absolute path)
		_indexFiles.push_back("index.html");
		_indexFiles.push_back("index.htm");
		//_indexFiles.push_back("index.php"); // TODO if uncommented, needs to be checked in StaticHandler::_serveFile
	}
}

/**
 * Set root path for this server.
 *
 * Syntax: `root path;`
 * Path must be an absolute path.
 * The last root directive overrides the previous one.
 *
 * Throws a runtime exception if:
 * - path is missing or is not an absolute path or an emtpy string.
 */
void	ServerBlock::_setRoot(Tokens const& tokens) {
	if (tokens.size() != 2)
		throw std::runtime_error("Format must be \"root path;\"");
	std::string root = tokens[1];
	if (root.empty())
		throw std::runtime_error("path is an empty string");
	if (!utils::isAbsolutePath(root)) {
		throw std::runtime_error("Not an absolute path: '" + root + "'");
	}
	_root = utils::normalizePath(root); // override previous root if already set
}

/**
 * Sets listen host:port pairs for this server.
 *
 * Syntax: `listen host1:port1 [host2:port2 ...];`
 * Example: `listen localhost:8080 80 1.1.1.1`
 *
 * Can accept several host:port pairs in one line if seperated by a space
 * If only a `host` is defined for a pair, port defaults to `80`.
 * If only a `port` is defined for a pair, host defaults to `0.0.0.0`.
 * `localhost` is the only server name supported for now.
 * For host, only IPv4 format is supported for now.
 *
 * Accepted syntax examples:
 * - listen 127.0.0.1:8080 → pair = "127.0.0.1:8080"
 * - no listen directive → pair = "0.0.0.0:80" (default set in constructor)
 * - listen 8080 → pair = "0.0.0.0:8080" (default host is 0.0.0.0)
 * - listen localhost:8080 → pair = "0.0.0.0:8080" ('localhost' is converted to default)
 *
 * Refused syntax examples (throw runtime_error):
 * - listen	127.0.1:8080   → host IP bad syntax
 * - listen 256.0.0.1:8080 → host IP out of range
 * - listen any_string_other_than_localhost:8080 → host IP bad syntax
 * - listen any_string → port invalid number, 'listen localhost' not supported without port
 * - listen 70000 → port out of range
 * - listen :8080 → missing host
 *
 * Throws a runtime exception if:
 * - `host:port` pair is empty, has an invalid syntax
 * - there is a port overflow
 */
void	ServerBlock::_setListen(Tokens const& tokens) {
	if (tokens.size() < 2)
		throw std::runtime_error("Should have at least one address or port");
	for (size_t i = 1; i < tokens.size(); ++i) {
		HostPortPair listen(tokens[i]); // throw
		_listen.insert(listen);
	}
}

/**
 * Sets client max body size limit for this location.
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
void	ServerBlock::_setClientMaxBodySize(Tokens const& tokens) {
	if (tokens.size() != 2)
		throw std::runtime_error("size is missing");
	size_t result = Config::parseSize(tokens[1]);
	if (result == 0)
		throw std::runtime_error("Must be more than 0 bytes:" + tokens[1]);
    _clientMaxBodySize = result;
	_isSetClientBodySize = true;
}

/**
 * Sets upload store directory for this server.
 *
 * Syntax: `upload_store path;`
 * Path can be absolute, or relative to the server root.
 * A server root is required for relative paths.
 *
 * If not set, uploads are disabled unless a location overrides it.
 *
 * Exception is thrown if:
 * - arguments are invalid
 * - the path is an empty string
 * - a relative path is given but no server root is set
 */
void	ServerBlock::_setUploadStore(Tokens const& tokens) {
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
 * Sets custom error pages for this server.
 *
 * Syntax: `error_page code1 [code2 ...] path;`
 * Error page path can be an absolute, or relative to root.
 *
 * If no custom error pages are set in this server, built-in ones are used.
 *
 * Exception is thrown if:
 * - arguments are invalid
 * - the path is an empty string
 * - a relative path is given but no root is set in this server
 * - an HTTP code is out of range (300-599)
 */
void	ServerBlock::_setErrorPages(Tokens const& tokens) {
	if (tokens.size() < 3)
		throw std::runtime_error("Should have at least 2 paths");
	// Check path (last argument)
	std::string path = tokens.back();
	if (path.empty())
		throw std::runtime_error("Error path is an empty string");
	if (utils::isRelativePath(path) && _root.empty())
		throw std::runtime_error("Path \"" + path + "\" is relative but root is not set");
	if (utils::isRelativePath(path))
		path = utils::pathsJoin(_root, path); // make absolute
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
 * Set index files for this server. Index files are served if a directory is requested.
 * If no index file is set for the requested directory and autoindex is on, a built-in directory index is served instead.
 *
 * Syntax: `index file1 [file2 ...];`
 * Index path can be aboslute, or relative to root.
 *
 * If no index files are set in this server, default index files are used.
 *
 * Exception is thrown if:
 * - arguments are invalid
 * - an index file is an empty string
 * - an index path is a relative path but no root is set
 * - duplicate index files
 */
void	ServerBlock::_setIndexFiles(Tokens const& tokens) {
	if (tokens.size() < 2)
		throw std::runtime_error("Should have 1 or more paths");
	_indexFiles.clear(); // clear default index files
	for (size_t j = 1; j < tokens.size(); ++j) {
		std::string path = tokens[j];
		if (path.empty())
			throw std::runtime_error("An index file is an emtpy string");
		if (utils::isRelativePath(path) && _root.empty())
			throw std::runtime_error("A relative path requires a server root");
		_indexFiles.push_back(path); // Relative paths are not resolved into absolute yet (done at runtime using request URI)
	}
	if (!utils::hasVectorUniqEntries(_indexFiles))
		throw std::runtime_error("Duplicate index file in server");
}

std::vector<LocationBlock> const&	ServerBlock::getLocations() const {
	return _locations;
}

std::string const&	ServerBlock::getRoot() const {
	return _root;
}

std::set<HostPortPair> const&	ServerBlock::getListen() const {
	return _listen;
}

/**
 * Defaults to `DEFAULT_MAX_CLIENT_BODY_SIZE` with a limit of `MAX_SIZE_T`.
 */
size_t	ServerBlock::getClientMaxBodySize() const {
	if (!_isSetClientBodySize) {
		if (DEFAULT_MAX_CLIENT_BODY_SIZE > MAX_SIZE_T)
			return MAX_SIZE_T;
		return DEFAULT_MAX_CLIENT_BODY_SIZE;
	}
	return _clientMaxBodySize;
}

std::string const&	ServerBlock::getUploadStore() const {
	return _uploadStore;
}

ErrorPages const&	ServerBlock::getErrorPages() const {
	return _errorPages;
}

std::vector<std::string> const&	ServerBlock::getIndexFiles() const {
	return _indexFiles;
}

std::ostream&	operator<<(std::ostream& os, ServerBlock const& rhs) {
	os << "- root: " << PrintableString(rhs.getRoot()) << "\n";

	std::set<HostPortPair> const& listen = rhs.getListen();
	os << "- listen: " << listen.size() << "\n";
	for (std::set<HostPortPair>::const_iterator it = listen.begin(); it != listen.end(); it++) {
		os << "  - " << PrintableString(it->getHost()) << " -> " << it->getPort() << "\n";
	}
	os << "- client_max_body_size: " << rhs.getClientMaxBodySize() << "\n";

	os << "- upload_store: " << PrintableString(rhs.getUploadStore()) << "\n";

	ErrorPages const& errorPages = rhs.getErrorPages();
	os << "- error_pages: " << errorPages.size() << "\n";
	for (ErrorPages::const_iterator it = errorPages.begin(); it != errorPages.end(); ++it)
		os << "  - " << it->first << " -> " << PrintableString(it->second) << "\n";

	std::vector<std::string> const& indexFiles = rhs.getIndexFiles();
	os << "- index_files: " << indexFiles.size() << "\n";
	for (size_t i = 0; i < indexFiles.size(); ++i)
		os << "  - " << PrintableString(indexFiles[i]) << "\n";

	std::vector<LocationBlock> const& locations = rhs.getLocations();
	os << "- locations: " << locations.size() << "\n";
	for (size_t i = 0; i < locations.size(); ++i)
		os << "  Location " << i << ":\n" << locations[i];

	return os;
}
