#include "config/ServerBlock.hpp"
#include "config/Config.hpp"
#include <stdexcept>
#include <iostream>
#include <cstdlib>

ServerBlock::ServerBlock(): _listen(0) {}

void	ServerBlock::parse(std::string const& content) {
	std::string token = "";
	std::vector<std::string> tokens;
	int braceDepth = 0;
	for (size_t i = 0; i < content.size(); ++i) {
		Config::_skipCommentIf(content, i);
		if (content[i] == '{') {
			++braceDepth;
			if (tokens[0] == "location" && tokens.size() == 2) {
				LocationBlock lb(tokens[1]);
				std::string blockContent = Config::_getBlockContent(content, i, braceDepth);
				lb.parse(blockContent);
				_addLocation(lb);
			if (tokens.size() <= 0)
				throw std::runtime_error("Unexpected '{'");
			} else {
				throw std::runtime_error("Invalid block: " + tokens[0]);
			}
			tokens.clear();
		} else if (content[i] == ';') {
			Config::_addTokenIf(token, tokens);
			if (tokens.size() > 0) {
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
				} else if (tokens[0] == "client_max_body_size" && tokens.size() == 2) {
					_clientMaxBodySize = tokens[1];
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
				} else {
					throw std::runtime_error("Unknown or malformed directive in server block: " + tokens[0]);
				}
			}
			tokens.clear();
		} else if (isspace(content[i])) {
			Config::_addTokenIf(token, tokens);
		} else {
			token += content[i];
		}
	}
}

void	ServerBlock::print() const {
	std::cout << "Server:\n"
		<< "- root: " << (_root.empty() ? "[empty]" : _root) << "\n"
		<< "- server_name: " << (_serverName.empty() ? "[empty]" : _serverName) << "\n"
		<< "- listen: " << _listen << "\n"
		<< "- client_max_body_size: " << (_clientMaxBodySize.empty() ? "[empty]" : _clientMaxBodySize) << "\n"
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

void	ServerBlock::_addLocation(LocationBlock const& location) {
	_locations.push_back(location);
}

std::string const&	ServerBlock::getRoot() const {
	return _root;
}

std::string const&	ServerBlock::getServerName() const {
	return _serverName;
}

int	ServerBlock::getListen() const {
	return _listen;
}

std::string const&	ServerBlock::getClientMaxBodySize() const {
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
