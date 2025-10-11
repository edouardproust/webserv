#include "config/ServerBlock.hpp"
#include "config/Config.hpp"
#include "utils/utils.hpp"
#include "constants.hpp"
#include <stdexcept>
#include <iostream>
#include <cstdlib>

ServerBlock::ServerBlock(std::string const& blockContent)
: _clientMaxBodySize(utils::parseSize("1M")) {
	_parse(blockContent);
	if (_listen.empty())
      	_listen.insert(IpPortPair("0.0.0.0", 80));
}

ServerBlock::ServerBlock(const ServerBlock &other) {
    *this = other;
}

ServerBlock& ServerBlock::operator=(ServerBlock const& other) {
	if (this != &other) {
		_root = other._root;
		_listen = other._listen;
		_clientMaxBodySize = other._clientMaxBodySize;
		_errorPages = other._errorPages;
		_indexFiles = other._indexFiles;
		_locations = other._locations;
	}
	return *this;
}

ServerBlock::~ServerBlock() {}

void	ServerBlock::_parse(std::string const& content) {
	std::string token = "";
	std::vector<std::string> tokens;
	bool inQuotes = false;
	int braceDepth = 0;
	for (size_t i = 0; i < content.size(); ++i) {
		if (content[i] == '#') {
			Config::_skipComment(content, i);
		} else if (content[i] == '{') {
			_parseBlock(tokens, content, i, braceDepth, inQuotes);
		} else if (content[i] == ';') {
			_parseDirective(token, tokens, inQuotes);
		} else if (isspace(content[i]) && !inQuotes) {
			Config::_addTokenIf(token, tokens);
		} else if (content[i] == '"') {
			inQuotes = !inQuotes;
		} else {
			token += content[i];
		}
	}
}

void	ServerBlock::_parseBlock(std::vector<std::string>& tokens, std::string const& content,
	size_t& i, int& braceDepth, bool inQuotes) {
		if (tokens.size() <= 0)
			throw std::runtime_error("Unexpected '{'");
		else if (inQuotes)
			throw std::runtime_error("Unexpected '{' in quoted string");
		++braceDepth;
		if (tokens[0] == "location" && tokens.size() == 2) {
			std::string blockContent = Config::_getBlockContent(content, i, braceDepth);
			LocationBlock lb(utils::normalizePath(tokens[1]), blockContent);
			_locations.push_back(lb);
		if (tokens.size() <= 0)
			throw std::runtime_error("Unexpected '{'");
		} else {
			throw std::runtime_error("Invalid block: " + tokens[0]);
		}
		// Additional blocks can be added here
		tokens.clear();
}

void	ServerBlock::_parseDirective(std::string& token, std::vector<std::string>& tokens, bool inQuotes) {
	Config::_addTokenIf(token, tokens);
	if (tokens.size() <= 0) {
		throw std::runtime_error("Unexpected ';'");
	} else if (inQuotes) {
		throw std::runtime_error("Unclosed quoted string in server block");
	} else if (tokens.size() > 0) {
		if (tokens[0] == "root" && tokens.size() == 2) {
			_root = utils::normalizePath(tokens[1]);
		} else if (tokens[0] == "listen" && tokens.size() == 2) {
			_listen.insert(_parseIpPortPair(tokens[1]));
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
				_clientMaxBodySize = utils::parseSize(tokens[1]);
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

IpPortPair ServerBlock::_parseIpPortPair(std::string const& token) {
	std::string host = "0.0.0.0"; // default if no host specified
	int port = 80; // default HTTP port
	std::string::size_type colonPos = token.find(':');
	if (colonPos != std::string::npos) {
		// Case: "IP:PORT"
		host = token.substr(0, colonPos);
		std::string portStr = token.substr(colonPos + 1);
		if (portStr.empty())
			throw std::runtime_error("Invalid listen directive: missing port after ':' -> " + token);
		std::istringstream iss(portStr);
        if (!(iss >> port))
			throw std::runtime_error("Invalid listen directive: bad port value -> " + token);
	} else {
		// Case: only a port number
		std::istringstream iss(token);
        if (!(iss >> port))
			throw std::runtime_error("Invalid listen directive: expected port number -> " + token);
	}
	// Check port validity
	if (port < 1 || port > 65535)
		throw std::runtime_error("Invalid listen port (must be between 1 and 65535): " + token);
	return IpPortPair(host, port);
}

void	ServerBlock::validate() const {
	if (!utils::isAbsolutePath(_root)) {
		throw std::runtime_error("Missing or invalid root directive in server");
	} else if (_clientMaxBodySize > MAX_CLIENT_BODY_SIZE)
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
		_locations[i].validate(*this);
	}
	if (!utils::hasVectorUniqEntries(allLocationPaths))
		throw std::runtime_error("Duplicate path across location blocks");
}

LocationBlock const&	ServerBlock::getBestLocationForPath(std::string const& path) const {
	const LocationBlock* best = NULL;
	size_t longest = 0;
	for (size_t i = 0; i < _locations.size(); ++i) {
		const std::string& locPath = _locations[i].getPath();
		if (path.compare(0, locPath.size(), locPath) == 0) { // prefix match
			if (locPath.size() > longest) {
				longest = locPath.size();
				best = &_locations[i];
			}
		}
	}
	if (!best)
		throw std::runtime_error("No matching location for path: " + path);
	return *best;
}


std::string const&	ServerBlock::getRoot() const {
	return _root;
}

std::set<IpPortPair> const&	ServerBlock::getListen() const {
	return _listen;
}

size_t	ServerBlock::getClientMaxBodySize() const {
	return _clientMaxBodySize;
}

std::map<int, std::string> const&	ServerBlock::getErrorPages() const {
	return _errorPages;
}

std::vector<std::string> const&	ServerBlock::getIndexFiles() const {
	return _indexFiles;
}

std::vector<LocationBlock> const&	ServerBlock::getLocations() const {
	return _locations;
}

std::ostream&	operator<<(std::ostream& os, ServerBlock const& rhs) {
	os << "Server:\n";
	os << "- root: " << (rhs.getRoot().empty() ? "[empty]" : rhs.getRoot()) << "\n";

	std::set<IpPortPair> const& listen = rhs.getListen();
	os << "- listen: " << listen.size() << "\n";
	for (std::set<IpPortPair>::const_iterator it = listen.begin(); it != listen.end(); it++) {
		os << "  - " << it->first << " -> " << it->second << "\n";
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
		os << locations[i];
	os << std::endl;
	return os;
}
