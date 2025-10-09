#include "config/LocationBlock.hpp"
#include "config/Config.hpp"
#include "utils/utils.hpp"
#include <stdexcept>
#include <iostream>
#include <cstdlib>

LocationBlock::LocationBlock(std::string const& path, std::string const& blockContent)
: _path(path), _return(std::make_pair(-1, "")), _clientMaxBodySizeSet(false) {
	_parse(blockContent);
}

LocationBlock::~LocationBlock() {}

void	LocationBlock::_parse(std::string const& content) {
	std::string token = "";
	std::vector<std::string> tokens;
	bool inQuotes = false;
	for (size_t i = 0; i < content.size(); ++i) {
		if (content[i] == '#') {
			Config::_skipComment(content, i);
		} else if (content[i] == '{') {
			throw std::runtime_error("Unexpected '{' in location block");
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

void	LocationBlock::_parseDirective(std::string& token, std::vector<std::string>& tokens, bool inQuotes) {
Config::_addTokenIf(token, tokens);
	if (tokens.size() <= 0) {
		throw std::runtime_error("Unexpected ';'");
	} else if (inQuotes) {
		throw std::runtime_error("Unclosed quoted string in location block");
	} else if (tokens[0] == "root" && tokens.size() == 2) {
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
	} else if (tokens[0] == "client_max_body_size" && tokens.size() == 2) {
		try {
			_clientMaxBodySizeSet = true;
			_clientMaxBodySize = utils::parseSize(tokens[1]);}
		catch (std::exception& e) {
			throw std::runtime_error("Invalid client_max_body_size: " + tokens[1]);
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
	// Additional directives can be added here
	tokens.clear();
}

void	LocationBlock::validate(ServerBlock const& server) const {
	if (!utils::isAbsolutePath(_path))
		throw std::runtime_error("Invalid location path: " + _path);
	else if (!_root.empty() && !utils::isAbsolutePath(_root)) {
		throw std::runtime_error("Invalid root " + _root + " in location " + _path);
	} else if (!_autoindex.empty() && _autoindex != "on" && _autoindex != "off")
		throw std::runtime_error("Invalid autoindex value in location " + _path + ": " + _autoindex);
	if (_return.first != -1 && (_return.first < 100 || _return.first > 599))
		throw std::runtime_error("Invalid return code in location " + _path + ": " + utils::toString(_return.first));
	if (server.getErrorPages().find(_return.first) != server.getErrorPages().end())
        throw std::runtime_error("Conflict: return " + utils::toString(_return.first) + " in location " + _path + " conflicts with error_page in server block");
	if (_clientMaxBodySizeSet && (_clientMaxBodySize <= 0 || _clientMaxBodySize > Config::MAX_CLIENT_BODY_SIZE))
		throw std::runtime_error("client_max_body_size is 0 or too high in location " + _path);
	if (!_cgiExtension.empty() && _cgiExtension[0] != '.')
		throw std::runtime_error("Invalid cgi_extension in location " + _path + ": " + _cgiExtension);
	if ((!_cgiRoot.empty() || !_cgiExtension.empty() || !_cgiExecutable.empty())
		&& (_cgiExtension.empty() || _cgiExecutable.empty()))
		throw std::runtime_error("Incomplete CGI configuration in location " + _path);
	for (std::set<std::string>::const_iterator it = _limitExcept.begin(); it != _limitExcept.end(); it++) {
		if (*it != "GET" && *it != "POST" && *it != "DELETE") //TODO: make it dynamic
			throw std::runtime_error("Not allowed limit_except method " + *it + " in location " + _path);
	}
	// Additional location block validation can be added here
}

void	LocationBlock::print() const {
	std::cout << "Location:\n"
		<< "- path: " << _path << "\n"
		<< "- root: " << (_root.empty() ? "[empty]" : _root) << "\n"
		<< "- autoindex: " << (_autoindex.empty() ? "[empty]" : _autoindex) << "\n"
		<< "- limit_except: " << _limitExcept.size() << "\n";
	for (std::set<std::string>::const_iterator it = _limitExcept.begin(); it != _limitExcept.end(); ++it)
		std::cout << "  - " << *it << "\n";
	if (_return.first != -1)
		std::cout << "- return: " << _return.first << " -> " << (_return.second.empty() ? "[empty]" : _return.second) << "\n";
	else
		std::cout << "- return: [empty]\n";
	if (_clientMaxBodySizeSet)
		std::cout << "- client_max_body_size: " << _clientMaxBodySize << "\n";
	else
		std::cout << "- client_max_body_size: [empty]\n";
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

std::string const	LocationBlock::getRoot(ServerBlock const& server) const {
	if (!_root.empty())
		return _root;
	if (!server.getRoot().empty())
		return server.getRoot();
	else
		return "index.html";
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

unsigned long	LocationBlock::getClientMaxBodySize(ServerBlock const& server) const {
	if (_clientMaxBodySizeSet)
		return _clientMaxBodySize;
	return server.getClientMaxBodySize();
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

std::vector<std::string> const&	LocationBlock::getIndexFiles(ServerBlock const& server) const {
	if (!_indexFiles.empty())
		return _indexFiles;
	return server.getIndexFiles();
}
