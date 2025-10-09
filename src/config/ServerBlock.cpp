#include "config/ServerBlock.hpp"
#include "config/Config.hpp"
#include "utils/utils.hpp"
#include <stdexcept>
#include <iostream>
#include <cstdlib>

ServerBlock::ServerBlock(std::string const& blockContent)
: _listen(0), _clientMaxBodySize(utils::parseSize("1M")) {
	_parse(blockContent);
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
		} else if (tokens[0] == "server_name" && tokens.size() == 2) {
			_serverName = tokens[1];
		} else if (tokens[0] == "listen" && tokens.size() == 2) {
			try {
				_listen = std::atoi(tokens[1].c_str());
			} catch (std::exception& e) {
				throw std::runtime_error("Invalid listen port: " + tokens[1]);
			}
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

void	ServerBlock::validate() const {
	if (_listen == 0)
		throw std::runtime_error("Missing listen directive in server " + _serverName);
	else if (_listen < 1 || _listen > 65535)
		throw std::runtime_error("Invalid listen port: " + utils::toString(_listen));
	else if (!utils::isAbsolutePath(_root)) {
		throw std::runtime_error("Missing or invalid root directive in server " + _serverName);
	} else if (_serverName.empty())
		throw std::runtime_error("Missing server_name directive in server " + _serverName);
	else if (_clientMaxBodySize > Config::MAX_CLIENT_BODY_SIZE)
		throw std::runtime_error("client_max_body_size is too high in server " + _serverName);
	for (size_t i = 0; i < _indexFiles.size(); i++) {
		if (_indexFiles[i].empty()) {
			throw std::runtime_error("Empty index file path in server " + _serverName);
		}
	}
	for (std::map<int, std::string>::const_iterator it = _errorPages.begin(); it != _errorPages.end(); it++) {
		if (it->first < 300 || it->first > 599)
			throw std::runtime_error("Invalid error_page code " + utils::toString(it->first) + " in server " + _serverName);
		if (it->second.empty())
			throw std::runtime_error("Empty path for error_page " + utils::toString(it->first) + " in server " + _serverName);
	}
	std::set<std::string> seenIndexFiles;
	for (size_t i = 0; i < _indexFiles.size(); ++i) {
		std::string currentIndexFile = _indexFiles[i];
		if (seenIndexFiles.find(currentIndexFile) != seenIndexFiles.end())
			throw std::runtime_error("Duplicate index file " + currentIndexFile + " in server " + _serverName);
	}
	std::set<std::string> seenLocPaths;
	for (size_t i = 0; i < _locations.size(); ++i) {
		std::string currentPath = _locations[i].getPath();
		if (seenLocPaths.find(currentPath) != seenLocPaths.end())
			throw std::runtime_error("Duplicate path across location blocks: " + currentPath);
		seenLocPaths.insert(currentPath);
	}
	// Additional server block validation can be added here
	for (size_t i = 0; i < _locations.size(); ++i) {
		_locations[i].validate(*this);
	}
}

void	ServerBlock::print() const {
	std::cout << "Server:\n"
		<< "- root: " << (_root.empty() ? "[empty]" : _root) << "\n"
		<< "- server_name: " << (_serverName.empty() ? "[empty]" : _serverName) << "\n"
		<< "- listen: " << _listen << "\n"
		<< "- client_max_body_size: " << _clientMaxBodySize << "\n"
		<< "- error_pages: " << _errorPages.size() << "\n";
	for (std::map<int, std::string>::const_iterator it = _errorPages.begin(); it != _errorPages.end(); ++it)
		std::cout << "  - " << it->first << " -> " << it->second << "\n";
	std::cout << "- index_files: " << _indexFiles.size() << "\n";
	for (size_t i = 0; i < _indexFiles.size(); ++i)
		std::cout << "  - " << _indexFiles[i] << "\n";
	std::cout << "- locations: " << _locations.size() << "\n";
	for (size_t i = 0; i < _locations.size(); ++i)
		_locations[i].print();
	std::cout << std::endl;
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

std::string const&	ServerBlock::getServerName() const {
	return _serverName;
}

unsigned int	ServerBlock::getListen() const {
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
