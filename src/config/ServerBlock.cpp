#include "config/ServerBlock.hpp"
#include "config/Config.hpp"
#include "utils/utils.hpp"
#include <stdexcept>
#include <iostream>
#include <cstdlib>

ServerBlock::ServerBlock() {} // Not used

ServerBlock::ServerBlock(std::string const& blockContent)
: _listen(0), _clientMaxBodySize(utils::parseSize("1M")) {
	_parse(blockContent);
}

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
			LocationBlock lb(tokens[1], blockContent);
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
			_root = tokens[1];
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
					_error_pages[code] = tokens.back();
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

void	ServerBlock::validate() {
	if (_listen == 0)
		throw std::runtime_error("Server block missing listen directive");
	else if (_listen < 1 || _listen > 65535)
		throw std::runtime_error("Invalid listen port: " + utils::to_string(_listen));
	else if (_root.empty())
		throw std::runtime_error("Server block missing root directive");
	else if (_serverName.empty())
		throw std::runtime_error("Server block missing server_name directive");
	else if (_clientMaxBodySize < 0 || _clientMaxBodySize > Config::MAX_CLIENT_BODY_SIZE)
		throw std::runtime_error("client_max_body_size is too high in server");
	for (size_t i = 0; i < _locations.size(); ++i) {
		_locations[i].validate(*this);
	}
	for (size_t i = 0; i < _indexFiles.size(); i++) {
		if (_indexFiles[i].empty()) {
			throw std::runtime_error("Server block has en empty index file path");
		}
	}
	std::set<std::string> locPaths;
	for (int i = 0; i < static_cast<int>(_locations.size()); ++i) {
		std::string path = _locations[i].getPath();
		if (locPaths.find(path) != locPaths.end())
			throw std::runtime_error("Duplicate path across location blocks: " + path);
		locPaths.insert(path);
	}
	// Additional server block validation can be added here
}

void	ServerBlock::print() const {
	std::cout << "Server:\n"
		<< "- root: " << (_root.empty() ? "[empty]" : _root) << "\n"
		<< "- server_name: " << (_serverName.empty() ? "[empty]" : _serverName) << "\n"
		<< "- listen: " << _listen << "\n"
		<< "- client_max_body_size: " << _clientMaxBodySize << "\n"
		<< "- error_pages: " << _error_pages.size() << "\n";
	for (std::map<int, std::string>::const_iterator it = _error_pages.begin(); it != _error_pages.end(); ++it)
		std::cout << "  - " << it->first << " -> " << it->second << "\n";
	std::cout << "- index_files: " << _indexFiles.size() << "\n";
	for (size_t i = 0; i < _indexFiles.size(); ++i)
		std::cout << "  - " << _indexFiles[i] << "\n";
	std::cout << "- locations: " << _locations.size() << "\n";
	for (size_t i = 0; i < _locations.size(); ++i)
		_locations[i].print();
	std::cout << std::endl;
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
	return _error_pages;
}

std::vector<std::string> const&	ServerBlock::getIndexFiles() const {
	return _indexFiles;
}

std::vector<LocationBlock> const&	ServerBlock::getLocations() const {
	return _locations;
}
