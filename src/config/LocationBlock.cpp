#include "config/LocationBlock.hpp"
#include "config/Config.hpp"
#include "utils/utils.hpp"
#include "http/Request.hpp"
#include "constants.hpp"
#include <stdexcept>
#include <iostream>
#include <cstdlib>

LocationBlock::LocationBlock(ServerBlock* server, std::string const& path, std::string const& blockContent)
: _server(server), _path(path), _return(std::make_pair(-1, "")), _clientMaxBodySizeSet(false) {
	_parse(blockContent);
}

LocationBlock::LocationBlock(const LocationBlock &other) {
    *this = other;
}

LocationBlock&	LocationBlock::operator=(LocationBlock const& other) {
	if (this != &other) {
		_server = other._server;
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

void	LocationBlock::validate() const {
	if (!utils::isAbsolutePath(_path))
		throw std::runtime_error("Invalid location path: " + _path);
	else if (!_root.empty() && !utils::isAbsolutePath(_root)) {
		throw std::runtime_error("Invalid root " + _root + " in location " + _path);
	} else if (!_autoindex.empty() && _autoindex != "on" && _autoindex != "off")
		throw std::runtime_error("Invalid autoindex value in location " + _path + ": " + _autoindex);
	if (_return.first != -1 && (_return.first < 100 || _return.first > 599))
		throw std::runtime_error("Invalid return code in location " + _path + ": " + utils::toString(_return.first));
	if (_server->getErrorPages().find(_return.first) != _server->getErrorPages().end())
        throw std::runtime_error("Conflict: return " + utils::toString(_return.first) + " in location " + _path + " conflicts with error_page in server block");
	if (_clientMaxBodySizeSet && (_clientMaxBodySize <= 0 || _clientMaxBodySize > MAX_CLIENT_BODY_SIZE))
		throw std::runtime_error("client_max_body_size is 0 or too high in location " + _path);
	for (std::set<std::string>::const_iterator it = _limitExcept.begin(); it != _limitExcept.end(); it++) {
		if (!Request::isSupportedMethod(*it))
			throw std::runtime_error("Not allowed limit_except method " + *it + " in location " + _path);
	}
	if (!utils::hasVectorUniqEntries(_indexFiles))
		throw std::runtime_error("Duplicate index file in location " + _path);
}

bool	LocationBlock::isCgiLocation() const {
	return !_cgi.empty();
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

std::string const&	LocationBlock::getAutoindex() const {
	return _autoindex;
}

std::set<std::string> const&	LocationBlock::getLimitExcept() const {
	return _limitExcept;
}

std::pair<int, std::string>	const&	LocationBlock::getReturn() const {
	return _return;
}

unsigned long	LocationBlock::getClientMaxBodySize() const {
	if (_clientMaxBodySizeSet)
		return _clientMaxBodySize;
	return _server->getClientMaxBodySize();
}

bool	LocationBlock::getClientMaxBodySizeSet() const {
	return _clientMaxBodySizeSet;
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

void LocationBlock::setServer(ServerBlock* server) {
	_server = server;
}

std::ostream&	operator<<(std::ostream& os, LocationBlock const& rhs) {
	std::string const in = "   "; // indentation
	os << in << "Location:\n";
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
