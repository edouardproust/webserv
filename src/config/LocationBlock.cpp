#include "config/LocationBlock.hpp"
#include "config/Config.hpp"
#include "utils/utils.hpp"
#include "constants.hpp"
#include "http/Request.hpp"
#include <stdexcept>
#include <iostream>
#include <cstdlib>

LocationBlock::LocationBlock(std::string const& path, std::string const& blockContent)
: _path(path), _return(std::make_pair(-1, "")), _clientMaxBodySizeSet(false) {
	_parse(blockContent);
}

LocationBlock::LocationBlock(const LocationBlock &other) {
    *this = other;
}

LocationBlock&	LocationBlock::operator=(LocationBlock const& other) {
	if (this != &other) {
		_path = other._path;
		_root = other._root;
		_autoindex= other._autoindex;
		_limitExcept= other._limitExcept;
		_return = other._return;
		_clientMaxBodySize = other._clientMaxBodySize;
		_clientMaxBodySizeSet = other._clientMaxBodySizeSet;
		_indexFiles = other._indexFiles;
		_cgi = other._cgi;
	}
	return *this;
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
	} else if (tokens[0] == "return" && (tokens.size() == 2 || tokens.size() == 3)) {
		_parseReturnDirective(tokens);
	} else if (tokens[0] == "client_max_body_size" && tokens.size() == 2) {
		try {
			_clientMaxBodySizeSet = true;
			_clientMaxBodySize = utils::parseSize(tokens[1]);}
		catch (std::exception& e) {
			throw std::runtime_error("Invalid client_max_body_size: " + tokens[1]);
		}
	} else if (tokens[0] == "cgi" && tokens.size() == 3) {
		_addCgiDirective(tokens[1], tokens[2]);
	} else if (tokens[0] == "index" && tokens.size() >= 2) {
		for (size_t j = 1; j < tokens.size(); ++j)
			_indexFiles.push_back(tokens[j]);
	} else {
		throw std::runtime_error("Unknown or malformed directive in location block: " + tokens[0]);
	}
	// Additional directives can be added here
	tokens.clear();
}


void	LocationBlock::_addCgiDirective(std::string extension, std::string executable) {
	if (extension.empty() || executable.empty())
		throw std::runtime_error("Invalid CGI directive: missing extension or executable");
    if (extension[0] != '.')
		throw std::runtime_error("Invalid CGI extension: must start with '.' -> " + extension);
    if (_cgi.find(extension) != _cgi.end())
		throw std::runtime_error("Duplicate CGI extension: " + extension);
	_cgi[extension] = executable;
}

void	LocationBlock::_parseReturnDirective(std::vector<std::string>& tokens) {
	if (_return.first != -1)
		throw std::runtime_error("Duplicate return directive in location");
	std::istringstream iss(tokens[1]);
	int code;
	bool isNumber = (iss >> code) && iss.eof();
	if (tokens.size() == 3) { // case "return <code> <url|text>;"
		if (!isNumber)
			throw std::runtime_error("Invalid return code: " + tokens[1]);
		if (code < 100 || code > 599)
			throw std::runtime_error("Return code out of range: " + tokens[1]);
		_return.first = code;
		_return.second = tokens[2];
	}
	else if (tokens.size() == 2) {
		if (isNumber) { // case "return <code>;"
			if (code < 100 || code > 599)
				throw std::runtime_error("Return code out of range: " + tokens[1]);
			_return.first = code;
			_return.second = "";
		} else { // case "return <url>;"
			_return.first = 302; // by default (nginx style)
			_return.second = tokens[1];
		}
	}
}


void	LocationBlock::validate(ServerBlock const& server) const {
	if (!utils::isAbsolutePath(_path))
		throw std::runtime_error("Invalid location path: " + _path);
	else if (!_root.empty() && !utils::isAbsolutePath(_root)) {
		throw std::runtime_error("Invalid root " + _root + " in location " + _path);
	} else if (!_autoindex.empty() && _autoindex != "on" && _autoindex != "off")
		throw std::runtime_error("Invalid autoindex value in location " + _path + ": " + _autoindex);
	if (server.getErrorPages().find(_return.first) != server.getErrorPages().end())
        throw std::runtime_error("Conflict: return " + utils::toString(_return.first) + " in location " + _path + " conflicts with error_page in server block");
	if (_clientMaxBodySizeSet && (_clientMaxBodySize <= 0 || _clientMaxBodySize > MAX_CLIENT_BODY_SIZE))
		throw std::runtime_error("client_max_body_size is 0 or too high in location " + _path);
	for (std::set<std::string>::const_iterator it = _limitExcept.begin(); it != _limitExcept.end(); it++) {
		if (!Request::isSupportedMethod(*it))
			throw std::runtime_error("Not supported limit_except method " + *it + " in location " + _path);
	}
	if (!utils::hasVectorUniqEntries(_indexFiles))
		throw std::runtime_error("Duplicate index file in location " + _path);
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
	std::cout << "- index_files: " << _indexFiles.size() << "\n";
	for (size_t i = 0; i < _indexFiles.size(); ++i)
		std::cout << "  - " << _indexFiles[i] << "\n";
	std::cout << "- cgi: " << _cgi.size() << "\n";
	for (CgiDirective::const_iterator it = _cgi.begin(); it != _cgi.end(); it++) {
		std::cout << "  - " << it->first << " -> " << it->second << "\n";
	}
}

std::string const&	LocationBlock::getPath() const {
	return _path;
}

std::string const	LocationBlock::getRoot(ServerBlock const& server) const {
	if (!_root.empty())
		return _root;
	return server.getRoot(); // returns "" if server::_root is not set either
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

CgiDirective const&	LocationBlock::getCgi() const {
	return _cgi;
}

std::vector<std::string> const&	LocationBlock::getIndexFiles(ServerBlock const& server) const {
	if (!_indexFiles.empty())
		return _indexFiles;
	return server.getIndexFiles();
}
