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
  _defaultLocation(LocationBlock(this)) {
	_parse(blockContent);
	if (_listen.empty())
      	_listen.insert(HostPortPair("0.0.0.0", 80));
}

ServerBlock::ServerBlock(const ServerBlock &other)
: _isSetClientBodySize(false),
  _defaultLocation(this) {
	*this = other;
}

ServerBlock& ServerBlock::operator=(ServerBlock const& other) {
    if (this != &other) {
        _root = other._root;
        _listen = other._listen;
        _clientMaxBodySize = other._clientMaxBodySize;
        _errorPages = other._errorPages;
        _indexFiles = other._indexFiles;
        _locations.clear();
        _locations.reserve(other._locations.size());
        for (size_t i = 0; i < other._locations.size(); ++i) {
            _locations.push_back(other._locations[i]);
            _locations.back().setServer(this);
        }
		_defaultLocation = other._defaultLocation;
        _defaultLocation.setServer(this);
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
		if (content[i] == '#') {
			Config::skipComment(content, i);
		} else if (content[i] == '{') {
			_parseBlock(tokens, content, i, braceDepth, inQuotes);
		} else if (content[i] == ';') {
			_parseDirective(token, tokens, inQuotes);
		} else if (isspace(content[i]) && !inQuotes) {
			Config::addTokenIf(token, tokens);
		} else if (content[i] == '"') {
			inQuotes = !inQuotes;
		} else {
			token += content[i];
		}
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
			_setLocations(tokens, content, i, braceDepth);
		// Additional blocks can be added here
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
	if (tokens.empty()) {
		throw std::runtime_error("Unexpected ';'");
	} else if (inQuotes) {
		throw std::runtime_error("Unclosed quoted string in server block");
	} else if (tokens.size() > 0) {
		if (tokens[0] == "root" && tokens.size() == 2) {
			_root = utils::normalizePath(tokens[1]);
		} else if (tokens[0] == "listen" && tokens.size() == 2) {
			_listen.insert(_parseHostPortPair(tokens[1]));
		} else if (tokens[0] == "error_page" && tokens.size() >= 3) {
			for (size_t j = 1; j < tokens.size() - 1; ++j) {
				try {
					int code = std::atoi(tokens[j].c_str());
					_errorPages[code] = tokens.back();
				} catch (std::exception& e) {
					throw std::runtime_error("Invalid error code: " + tokens[j]);
				}
			}
		} else if (tokens[0] == "index" && tokens.size() >= 2) {
			for (size_t j = 1; j < tokens.size(); ++j)
				_indexFiles.push_back(tokens[j]);
		} else if (tokens[0] == "client_max_body_size" && tokens.size() == 2) {
			try {
				_clientMaxBodySize = Config::parseSize(tokens[1]); // throw if empty, invalid syntax or overflow
			} catch (std::exception& e) {
				throw std::runtime_error("Invalid client_max_body_size: " + tokens[1]);
			}
		} else {
			throw std::runtime_error("Unknown or malformed directive in server block: " + tokens[0]);
		}
		// Additional directives can be added here
	}
	tokens.clear();
}

/**
 * May throw a std::runtime_error() exception.
 *
 * Accepted syntax examples:
 * - listen 127.0.0.1:8080 → pair = "127.0.0.1:8080"
 * - no listen directive → pair = "0.0.0.0:80" (default set in constructor)
 * - listen 8080 → pair = "0.0.0.0:8080" (default host is 0.0.0.0)
 * - listen localhost:8080 → pair = "0.0.0.0:8080" ('localhost' is converted to default)
 * Refused syntax examples (throw runtime_error):
 * - listen	127.0.1:8080   → host IP bad syntax
 * - listen 256.0.0.1:8080 → host IP out of range
 * - listen any_string_other_than_localhost:8080 → host IP bad syntax
 * - listen any_string → port invalid number, 'listen localhost' not supported without port
 * - listen 70000 → port out of range
 * - listen :8080 → missing host
 */
HostPortPair ServerBlock::_parseHostPortPair(std::string const& token) {
	std::string host;
	int port = -1;
	std::string::size_type colonPos = token.rfind(':');
	if (colonPos != std::string::npos) {
		// Case: "IP:PORT"
		host = token.substr(0, colonPos);
		std::string portStr = token.substr(colonPos + 1);
		if (portStr.empty())
			throw std::runtime_error("Missing host in listen directive: " + token);
		std::istringstream iss(portStr);
        if (!(iss >> port))
			throw std::runtime_error("Invalid listen port value: " + token);
	} else {
		// Case: only a port is given
		std::istringstream iss(token);
        if (iss >> port) // this is a numeric port
			host = "0.0.0.0";
		else // this is a host name
			host = token;
	}
	return HostPortPair(host, port);
}

/**
 * May throw a std::runtime_error() exception.
 */
void	ServerBlock::_setLocations(Tokens const& tokens, std::string const& content, size_t& i, int& braceDepth) {
	if (tokens.size() != 2)
		throw std::runtime_error("Path is missing");
	std::string blockContent = Config::getBlockContent(content, i, braceDepth);
	LocationBlock lb(this, tokens[1], blockContent);
	_locations.push_back(lb);
}

/**
 * May throw a std::runtime_error() exception.
 */
void	_setRoot(Tokens const& token) {

}

/**
 * May throw a std::runtime_error() exception.
 */
void	_setListen(Tokens const& tokens) {

}

/**
 * May throw a std::runtime_error() exception.
 */
void	_setClientMaxBodySize(Tokens const& tokens) {

}

/**
 * May throw a std::runtime_error() exception.
 */
void	_setErrorPages(Tokens const& tokens) {

}

/**
 * May throw a std::runtime_error() exception.
 */
void	_setIndexFiles(Tokens const& tokens) {

}

/**
 * May throw a std::runtime_error() exception.
 */
void	ServerBlock::crossServersValidation() const {
	if (!utils::isAbsolutePath(_root)) {
		throw std::runtime_error("Missing or invalid root directive in server");
	} else if (_clientMaxBodySize > MAX_SIZE_T)
		throw std::runtime_error("client_max_body_size is too high in server");
	for (size_t i = 0; i < _indexFiles.size(); i++) {
		if (_indexFiles[i].empty()) {
			throw std::runtime_error("Empty index file path in server");
		}
	}
	for (std::map<int, std::string>::const_iterator it = _errorPages.begin(); it != _errorPages.end(); it++) {
		if (it->first < 300 || it->first > 599)
			throw std::runtime_error("Invalid error_page code " + utils::toString(it->first) + " in server");
		if (it->second.empty())
			throw std::runtime_error("Empty path for error_page " + utils::toString(it->first) + " in server");
	}
	if (!utils::hasVectorUniqEntries(_indexFiles))
			throw std::runtime_error("Duplicate index file in server");

	// Locations content validation
	std::vector<std::string> allLocationPaths;
	for (size_t i = 0; i < _locations.size(); ++i) {
		// collect all location paths for global duplicate check
		allLocationPaths.push_back(_locations[i].getPath());
		// validate this location individually
		_locations[i].crossDirectivesValidation();
	}
	if (!utils::hasVectorUniqEntries(allLocationPaths))
		throw std::runtime_error("Duplicate path across location blocks");
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
