#include "config/ServerBlock.hpp"
#include "config/Config.hpp"
#include "utils/utils.hpp"
#include "constants.hpp"
#include <stdexcept>
#include <iostream>
#include <cstdlib>

/**
 * May throw an std exception.
 */
ServerBlock::ServerBlock()
: _isSetClientBodySize(false),
  _defaultLocation(LocationBlock(this))
{}

/**
 * May throw a std exception.
 *
 * If no listen directive in the server block: add "listen 0.0.0.0:80;"
 */
ServerBlock::ServerBlock(std::string const& blockContent)
: _isSetClientBodySize(false),
  _defaultLocation(LocationBlock(this))
{
	_parse(blockContent); // throw
	// set default listen directive if none exist after parsing
	if (_listen.empty())
      	_listen.insert(HostPortPair("0.0.0.0:80"));
}

ServerBlock::ServerBlock(const ServerBlock &other)
: _isSetClientBodySize(false), _defaultLocation(this)
{
	*this = other;
}

ServerBlock& ServerBlock::operator=(ServerBlock const& other) {
    if (this != &other) {
        _defaultLocation.setServer(this);
        _root = other._root;
        _listen = other._listen;
        _clientMaxBodySize = other._clientMaxBodySize;
        _errorPages = other._errorPages;
        _indexFiles = other._indexFiles;
		// locations
		_locations.clear();
        _locations.reserve(other._locations.size());
        for (size_t i = 0; i < other._locations.size(); ++i) {
            _locations.push_back(other._locations[i]);
            _locations.back().setServer(this);
        }
		_defaultLocation = other._defaultLocation;
    }
    return *this;
}

ServerBlock::~ServerBlock() {}

/**
 * May throw a std exception.
 */
void	ServerBlock::_parse(std::string const& content) {
	std::string token = "";
	Tokens tokens;
	bool inQuotes = false;
	int braceDepth = 0;
	for (size_t i = 0; i < content.size(); ++i) {
		if (content[i] == '#')
			Config::skipComment(content, i);
		else if (content[i] == '{')
			_parseBlock(tokens, content, i, braceDepth, inQuotes);
		else if (content[i] == ';')
			_parseDirective(token, tokens, inQuotes);
		else if (isspace(content[i]) && !inQuotes)
			Config::addTokenIf(token, tokens);
		else if (content[i] == '"')
			inQuotes = !inQuotes;
		else
			token += content[i];
	}
}

/**
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
		else
			throw std::runtime_error("Unsupported block: " + tokens[0]);
	} catch (std::exception& e) {
		throw std::runtime_error(blockName + ": " + e.what()); // wrap error msg with the block name
	}
	tokens.clear();
}

/**
 * May throw a std::runtime_error() exception.
 */
void	ServerBlock::_parseDirective(std::string& token, Tokens& tokens, bool inQuotes) {
	Config::addTokenIf(token, tokens);
	if (tokens.empty())
		throw std::runtime_error("Unexpected ';'");
	else if (inQuotes)
		throw std::runtime_error("Unclosed quoted string in server block");

	std::string directiveName = tokens[0];
	try {
		if (tokens[0] == "root")
			_setRoot(tokens);
		else if (tokens[0] == "listen")
			_setListen(tokens);
		else if (tokens[0] == "client_max_body_size")
			_setClientMaxBodySize(tokens);
		else if (tokens[0] == "error_page")
			_setErrorPages(tokens);
		else if (tokens[0] == "index")
			_setIndexFiles(tokens);
		// -- additional directives can be added here --
		else {
			throw std::runtime_error("Unsupported directive");
		}
	} catch (std::exception& e) {
		throw std::runtime_error(directiveName + ": " + e.what()); // wrap error msg with the directive name
	}

	tokens.clear();
}

/**
 * May throw a std::runtime_error() exception.
 */
void	ServerBlock::_addLocation(Tokens const& tokens, std::string const& content, size_t& i, int& braceDepth) {
	if (tokens.size() != 2)
		throw std::runtime_error("Invalid block declaration (should be 'location /path {...}')");

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
 * May throw a std::runtime_error() exception.
 */
void	ServerBlock::_setRoot(Tokens const& tokens) {
	if (tokens.size() != 2)
		throw std::runtime_error("Should have 2 arguments");
	std::string root = tokens[1];
	if (root.empty())
		throw std::runtime_error("Value is an empty string");
	if (!utils::isAbsolutePath(root))
		throw std::runtime_error("Not an absolute path: '" + root + "'");
	_root = utils::normalizePath(root);
}

/**
 * May throw a std::runtime_error() exception.
 *
 * Can accept several host:port pairs in one line if seperated by a space:
 * listen localhost:8080 1.1.1.1:80
 * Only supports IPv4 for now // TODO support IPv6 also?
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
 */
void	ServerBlock::_setListen(Tokens const& tokens) {
	if (tokens.size() < 2)
		throw std::runtime_error("Should have at least one address or port");
	for (size_t i = 1; i < tokens.size(); ++i) {
		HostPortPair listen(tokens[i]); // throw if empty, invalid syntax, port overflow, etc.
		_listen.insert(listen);
	}
}

/**
 * May throw a std::runtime_error() exception.
 */
void	ServerBlock::_setClientMaxBodySize(Tokens const& tokens) {
	if (tokens.size() != 2)
		throw std::runtime_error("Should have 2 arguments");
	size_t result = Config::parseSize(tokens[1]); // throw if empty, invalid syntax or overflow
	if (result == 0)
		throw std::runtime_error("Must be more than 0 bytes:" + tokens[1]);
    _clientMaxBodySize = result;
	_isSetClientBodySize = true;
}

/**
 * May throw an exception.
 */
void	ServerBlock::_setErrorPages(Tokens const& tokens) {
	if (tokens.size() < 3)
		throw std::runtime_error("Should have at least 3 arguments");

	// Check path (last argument)
	std::string path = tokens.back();
	if (!utils::isAbsolutePath(path)) // also checks if is an empty str
		throw std::runtime_error("Not an absolute path: '" + path + "'");
	// TODO in Router: check if path exists

	for (size_t j = 1; j < tokens.size() - 1; ++j) {
		// Check each HTTP status codes
		std::string codeStr = tokens[j];
		size_t code = utils::toSizeT(codeStr); // throw if empty string, not an number or size_t overflow
		if (code < 300 || code > 599)
			throw std::out_of_range("Invalid HTTP code: " + codeStr);
		// Add to map (overrides value of existing codes)
		_errorPages[code] = path;
	}
}

/**
 * May throw a std::runtime_error() exception.
 */
void	ServerBlock::_setIndexFiles(Tokens const& tokens) {
	if (tokens.size() < 2)
		throw std::runtime_error("Should have 2 or more arguments");
	for (size_t j = 1; j < tokens.size(); ++j) {
		if (tokens[j].empty())
			throw std::runtime_error("An index file is an emtpy string");
		// TODO in Router: check if file exists
		_indexFiles.push_back(tokens[j]);
	}
	if (!utils::hasVectorUniqEntries(_indexFiles))
		throw std::runtime_error("Duplicate index file in server");
}

std::vector<LocationBlock> const&	ServerBlock::getLocations() const {
	return _locations;
}

LocationBlock const&	ServerBlock::getDefaultLocation() const {
	return _defaultLocation;
}

std::string const&	ServerBlock::getRoot() const {
	return _root;
}

std::set<HostPortPair> const&	ServerBlock::getListen() const {
	return _listen;
}

size_t	ServerBlock::getClientMaxBodySize() const {
	if (!_isSetClientBodySize) {
		if (DEFAULT_MAX_CLIENT_BODY_SIZE > MAX_SIZE_T)
			return MAX_SIZE_T;
		return DEFAULT_MAX_CLIENT_BODY_SIZE;
	}
	return _clientMaxBodySize;
}

std::map<int, std::string> const&	ServerBlock::getErrorPages() const {
	return _errorPages;
}

std::vector<std::string> const&	ServerBlock::getIndexFiles() const {
	return _indexFiles;
}

std::ostream&	operator<<(std::ostream& os, ServerBlock const& rhs) {
	os << "- root: " << (rhs.getRoot().empty() ? "[empty]" : rhs.getRoot()) << "\n";

	std::set<HostPortPair> const& listen = rhs.getListen();
	os << "- listen: " << listen.size() << "\n";
	for (std::set<HostPortPair>::const_iterator it = listen.begin(); it != listen.end(); it++) {
		os << "  - " << it->getHost() << " -> " << it->getPort() << "\n";
	}
	os << "- client_max_body_size: " << rhs.getClientMaxBodySize() << "\n";

	std::map<int, std::string> const& errorPages = rhs.getErrorPages();
	os << "- error_pages: " << errorPages.size() << "\n";
	for (std::map<int, std::string>::const_iterator it = errorPages.begin(); it != errorPages.end(); ++it)
		os << "  - " << it->first << " -> " << it->second << "\n";

	std::vector<std::string> const& indexFiles = rhs.getIndexFiles();
	os << "- index_files: " << indexFiles.size() << "\n";
	for (size_t i = 0; i < indexFiles.size(); ++i)
		os << "  - " << indexFiles[i] << "\n";

	std::vector<LocationBlock> const& locations = rhs.getLocations();
	os << "- locations: " << locations.size() << "\n";
	for (size_t i = 0; i < locations.size(); ++i)
		os << "  Location " << i << ":\n" << locations[i];

	return os;
}
