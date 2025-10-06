#include "config/LocationBlock.hpp"
#include "config/Config.hpp"
#include <stdexcept>
#include <iostream>
#include <cstdlib>

LocationBlock::LocationBlock() {}

LocationBlock::LocationBlock(std::string const& path): ServerBlock(),
	_path(path), _return(std::make_pair(-1, "")) {}

void	LocationBlock::parse(std::string& content) {
	std::string token = "";
	std::vector<std::string> tokens;
	bool inQuotes = false;
	for (size_t i = 0; i < content.size(); ++i) {
		Config::_skipCommentIf(content, i);
		if (content[i] == '{') {
			throw std::runtime_error("Unexpected '{' in location block");
		} else if (content[i] == ';') {
			Config::_addTokenIf(token, tokens);
			if (tokens.size() <= 0) {
				throw std::runtime_error("Unexpected ';'");
			}
			else if (tokens[0] == "root" && tokens.size() == 2) {
				_root = tokens[1];
			} else if (tokens[0] == "autoindex" && tokens.size() == 2) {
				_autoindex = tokens[1];
			} else if (tokens[0] == "limit_except" && tokens.size() >= 2) {
				for (size_t j = 1; j < tokens.size(); ++j)
					_limitExcept.insert(tokens[j]);
			} else if (tokens[0] == "return" && tokens.size() == 3) {
				if (_return.first != -1)
					throw std::runtime_error("Duplicate return directive in location");
				try {
					_return.first = std::atoi(tokens[1].c_str());
					_return.second = tokens[2];
				} catch (std::exception& e) {
					throw std::runtime_error("Invalid return code: " + tokens[1]);
				}
			} else if (tokens[0] == "cgi_root" && tokens.size() == 2) {
				_cgiRoot = tokens[1];
			} else if (tokens[0] == "cgi_extension" && tokens.size() == 2) {
				_cgiExtension = tokens[1];
			} else if (tokens[0] == "cgi_executable" && tokens.size() == 2) {
				_cgiExecutable = tokens[1];
			} else if (tokens[0] == "index" && tokens.size() >= 2) {
				for (size_t j = 1; j < tokens.size(); ++j)
					_indexFiles.push_back(tokens[j]);
			} else {
				throw std::runtime_error("Unknown or malformed directive in location block: " + tokens[0]);
			}
			tokens.clear();
		} else if (isspace(content[i]) && !inQuotes) {
			Config::_addTokenIf(token, tokens);
		} else if (content[i] == '"') {
			inQuotes = !inQuotes;
		} else {
			token += content[i];
		}
	}
}

void	LocationBlock::print() const {
	std::cout << "Location:\n"
		<< "- path: " << _path << "\n"
		<< "- root: " << (_root.empty() ? "[empty]" : _root) << "\n"
		<< "- autoindex: " << (_autoindex.empty() ? "[empty]" : _autoindex) << "\n"
		<< "- allowed_methods: " << _limitExcept.size() << "\n";
	for (std::set<std::string>::const_iterator it = _limitExcept.begin(); it != _limitExcept.end(); ++it)
		std::cout << "  - " << *it << "\n";
	if (_return.first != -1)
		std::cout << "- return: " << _return.first << " -> " << (_return.second.empty() ? "[empty]" : _return.second) << "\n";
	else
		std::cout << "- return: [empty]\n";
	std::cout << "- cgi_root: " << (_cgiRoot.empty() ? "[empty]" : _cgiRoot) << "\n"
		<< "- cgi_extension: " << (_cgiExtension.empty() ? "[empty]" : _cgiExtension) << "\n"
		<< "- cgi_executable: " << (_cgiExecutable.empty() ? "[empty]" : _cgiExecutable) << "\n"
		<< "- index_files: " << _indexFiles.size() << "\n";
	for (size_t i = 0; i < _indexFiles.size(); ++i)
		std::cout << "  - " << _indexFiles[i] << "\n";
}

std::string const&	LocationBlock::getPath() const {
	return _path;
}

std::string const&	LocationBlock::getRoot() const {
	return _root;
}

std::string const&	LocationBlock::getAutoindex() const {
	return _autoindex;
}

std::set<std::string> const&	LocationBlock::getAllowedMethods() const {
	return _limitExcept;
}

std::pair<int, std::string>	const&	LocationBlock::getReturn() const {
	return _return;
}

//TODO verify this
std::string const&	LocationBlock::getClientMaxBodySize() const {
	if (!_clientMaxBodySize.empty())
		return _clientMaxBodySize;
	return ServerBlock::getClientMaxBodySize();
}

std::string const&	LocationBlock::getCgiRoot() const {
	return _cgiRoot;
}

std::string const&	LocationBlock::getCgiExtension() const {
	return _cgiExtension;
}

std::string const&	LocationBlock::getCgiExecutable() const {
	return _cgiExecutable;
}

std::vector<std::string> const&	LocationBlock::getIndexFiles() const {
	return _indexFiles;
}
