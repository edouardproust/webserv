#include "config/LocationBlock.hpp"
#include "config/Config.hpp"
#include "utils/utils.hpp"
#include "http/Request.hpp"
#include "constants.hpp"
#include <stdexcept>
#include <iostream>
#include <cstdlib>

LocationBlock::LocationBlock(ServerBlock* server)
: _server(server), _path("/"), _return(std::make_pair(-1, "")), _isSetClientMaxBodySize(false)
{}

/**
 * May throw a runtime_error() exception.
 */
LocationBlock::LocationBlock(ServerBlock* server, std::string& path, std::string const& blockContent)
: _server(server), _return(std::make_pair(-1, "")), _isSetClientMaxBodySize(false) {
	_setPath(path);
	_parse(blockContent);
}

LocationBlock::LocationBlock(const LocationBlock &other)
: _server(other._server),
  _path(other._path),
  _root(other._root),
  _autoindex(other._autoindex),
  _limitExcept(other._limitExcept),
  _return(other._return),
  _clientMaxBodySize(other._clientMaxBodySize),
  _isSetClientMaxBodySize(other._isSetClientMaxBodySize),
  _indexFiles(other._indexFiles),
  _cgi(other._cgi)
{}

LocationBlock&	LocationBlock::operator=(LocationBlock const& other) {
	if (this != &other) {
		_server = other._server;
		_path = other._path;
		_root = other._root;
		_autoindex= other._autoindex;
		_limitExcept= other._limitExcept;
		_return = other._return;
		_clientMaxBodySize = other._clientMaxBodySize;
		_isSetClientMaxBodySize = other._isSetClientMaxBodySize;
		_indexFiles = other._indexFiles;
		_cgi = other._cgi;
	}
	return *this;
}

LocationBlock::~LocationBlock() {}

/**
 * May throw a runtime_error() exception.
 */
void	LocationBlock::_parse(std::string const& content) {
	std::string token = "";
	Tokens tokens;
	bool inQuotes = false;
	for (size_t i = 0; i < content.size(); ++i) {
		if (content[i] == '#')
			Config::skipComment(content, i);
		else if (content[i] == '{')
			throw std::runtime_error("Unexpected '{'");
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
 * May throw a runtime_error() exception.
 */
void	LocationBlock::_parseDirective(std::string& token, std::vector<std::string>& tokens, bool inQuotes) {
	Config::addTokenIf(token, tokens);
	if (tokens.empty())
		throw std::runtime_error("Unexpected ';'");
	else if (inQuotes)
		throw std::runtime_error("Unclosed quoted string");

	std::string directiveName = tokens[0];
	try {
		if (directiveName == "root")
			_setRoot(tokens);
		else if (directiveName == "autoindex")
			_setAutoindex(tokens);
		else if (directiveName == "limit_except")
			_setLimitExcept(tokens);
		else if (directiveName == "return")
			_setReturn(tokens);
		else if (directiveName == "client_max_body_size")
			_setClientMaxBodySize(tokens);
		else if (directiveName == "index")
			_setIndexFiles(tokens);
		else if (directiveName == "cgi")
			_setCgi(tokens);
		// -- additional directives can be added here --
		else
			throw std::runtime_error("Unsupported directive");
	} catch (std::exception& e) {
		throw std::runtime_error(directiveName + ": " + e.what()); // wrap error msg with the directive name
	}

	tokens.clear();
}

void	LocationBlock::_setPath(std::string& path) {
	if (!utils::isAbsolutePath(path))
		throw std::runtime_error("Not an absolute path: '" + path + "'");
	_path = utils::normalizePath(path);
}

void	LocationBlock::setServer(ServerBlock* server) {
	_server = server;
}

/**
 * May throw a runtime_error() exception.
 */
void	LocationBlock::_setRoot(Tokens const& tokens) {
	if (tokens.size() != 2)
		throw std::runtime_error("Should have 2 arguments");
	std::string root = tokens[1];
	if (root.empty())
		throw std::runtime_error("Value is an empty string");
	if (!utils::isAbsolutePath(root))
		throw std::runtime_error("Not an absolute path: '" + root + "'");
	_root = root;
}

/**
 * May throw a runtime_error() exception.
 */
void	LocationBlock::_setAutoindex(Tokens const& tokens) {
	if (tokens.size() != 2)
		throw std::runtime_error("Should have 2 arguments");
	if (tokens[1].empty())
		throw std::runtime_error("Value is an empty string");
	if (_autoindex != "on" && _autoindex != "off")
		throw std::runtime_error("Value should be \"on\" or \"off\"");
	_autoindex = tokens[1];
}

/**
 * May throw a runtime_error() exception.
 */
void	LocationBlock::_setLimitExcept(Tokens const& tokens) {
	if (tokens.size() < 2)
		throw std::runtime_error("Should have at least 2 arguments");
	if (tokens[1].empty())
		throw std::runtime_error("Value is an empty string");
	for (std::set<std::string>::const_iterator it = _limitExcept.begin(); it != _limitExcept.end(); it++) {
		if (!Request::isSupportedMethod(*it))
			throw std::runtime_error("Not supported method: " + *it);
	}
	_autoindex = tokens[1];
}

/**
 * May throw a runtime_error() exception.
 */
void LocationBlock::_setReturn(Tokens const& tokens) {
	if (_return.first != -1)
			throw std::runtime_error("Duplicate directive");
	if (tokens.size() < 2 || tokens.size() > 3)
			throw std::runtime_error("Should have 2 or 3 arguments");
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
 * May throw a runtime_error() exception.
 */
void	LocationBlock::_setClientMaxBodySize(Tokens const& tokens) {
	if (tokens.size() != 2)
		throw std::runtime_error("Should have 2 arguments");
	size_t result = Config::parseSize(tokens[1]); // throw exception if empty, wrong syntax or overflow
	if (result == 0)
		throw std::runtime_error("Must be more than 0 bytes:" + tokens[1]); // Size cannot be 0
    _clientMaxBodySize = result;
    _isSetClientMaxBodySize = true;
}

/**
 * May throw a runtime_error() exception.
 */
void	LocationBlock::_setIndexFiles(Tokens const& tokens) {
	if (tokens.size() < 2)
		throw std::runtime_error("Should have 2 or more arguments");
	for (size_t j = 1; j < tokens.size(); ++j) {
		if (tokens[j].empty())
			throw std::runtime_error("An index value is an empty string");
		// TODO: check that each index file is accessible ?
		_indexFiles.push_back(tokens[j]);
	}
	if (!utils::hasVectorUniqEntries(_indexFiles))
		throw std::runtime_error("Duplicated index files");
}

/**
 * May throw a runtime_error() exception.
 */
void	LocationBlock::_setCgi(Tokens const& tokens) {
	if (tokens.size() != 3)
		throw std::runtime_error("Should have 3 arguments");
	if (tokens[1].empty())
		throw std::runtime_error("Extension is an empty string");
	if (tokens[2].empty())
		throw std::runtime_error("Executable is an empty string");
	std::string extension = tokens[1], executable = tokens[2];
	if (extension.empty() || executable.empty())
		throw std::runtime_error("Extension is an empty string");
	if (executable.empty())
		throw std::runtime_error("Executable path is an empty string");
    if (extension[0] != '.')
		throw std::runtime_error("Extension must start with a dot: " + extension);
    if (_cgi.find(extension) != _cgi.end())
		throw std::runtime_error("Duplicate extension: " + extension);
	if (!utils::isAccessibleDirectory(executable)) // TODO test this check works
		throw std::runtime_error("Executable cannot be accessed on host: " + executable);
	_cgi[extension] = executable;
}

ServerBlock*	LocationBlock::getServer() const {
	return _server;
}

std::string const&	LocationBlock::getPath() const {
	return _path;
}

std::string const	LocationBlock::getRoot() const {
	if (!_root.empty())
		return _root;
	return _server->getRoot(); // returns "" if server::_root is not set either
}

std::string const	LocationBlock::getAutoindex() const {
	if (_autoindex.empty())
		return "on";
	return _autoindex;
}

std::set<std::string> const&	LocationBlock::getLimitExcept() const {
	return _limitExcept;
}

std::pair<int, std::string>	const&	LocationBlock::getReturn() const {
	return _return;
}

unsigned long	LocationBlock::getClientMaxBodySize() const {
	if (_isSetClientMaxBodySize)
		return _clientMaxBodySize;
	return _server->getClientMaxBodySize();
}

bool	LocationBlock::getClientMaxBodySizeSet() const {
	return _isSetClientMaxBodySize;
}

CgiDirective const&	LocationBlock::getCgi() const {
	return _cgi;
}

std::vector<std::string> const&	LocationBlock::getIndexFiles() const {
	if (!_indexFiles.empty())
		return _indexFiles;
	return _server->getIndexFiles();
}

std::string const	LocationBlock::getCgiExecutor(std::string const& extension) const {
	CgiDirective::const_iterator it = _cgi.find(extension);
	if (it != _cgi.end())
		return it->second;
	return "";
}

bool	LocationBlock::isCgiLocation() const {
	return !_cgi.empty();
}

bool	LocationBlock::isRedirectionLocation() const {
	return _return.first != -1 && !_return.second.empty();
}


std::ostream&	operator<<(std::ostream& os, LocationBlock const& rhs) {
	std::string const in = "  "; // indentation
	os << in << "- path: " << rhs.getPath() << "\n";
	os << in << "- root: " << (rhs.getRoot().empty() ? "[empty]" : rhs.getRoot()) << "\n";
	os << in << "- autoindex: " << (rhs.getAutoindex().empty() ? "[empty]" : rhs.getAutoindex()) << "\n";

	std::set<std::string> const& limitExcept = rhs.getLimitExcept();
	os << in << "- limit_except: " << limitExcept.size() << "\n";
	for (std::set<std::string>::const_iterator it = limitExcept.begin(); it != limitExcept.end(); ++it)
		os << in << "  - " << *it << "\n";

	std::pair<int, std::string> const& retrn = rhs.getReturn();
	if (retrn.first != -1)
		os << in << "- return: " << retrn.first << " -> " << (retrn.second.empty() ? "[empty]" : retrn.second) << "\n";
	else
		os << in << "- return: [empty]\n";

	if (rhs.getClientMaxBodySizeSet())
		os << in << "- client_max_body_size: " << rhs.getClientMaxBodySize() << "\n";
	else
		os << in << "- client_max_body_size: [empty]\n";

	std::vector<std::string> const& indexFiles = rhs.getIndexFiles();
	os << in << "- index_files: " << indexFiles.size() << "\n";
	for (size_t i = 0; i < indexFiles.size(); ++i)
		os << in << "  - " << indexFiles[i] << "\n";

	CgiDirective const& cgi = rhs.getCgi();
	os << in << "- cgi: " << cgi.size() << "\n";
	for (CgiDirective::const_iterator it = cgi.begin(); it != cgi.end(); it++) {
		os << in << "  - " << it->first << " -> " << it->second << "\n";
	}

	return os;
}
