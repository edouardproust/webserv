#include "config/Config.hpp"
#include "config/ServerBlock.hpp"
#include "utils/utils.hpp"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>

Config::Config(std::string const& filePath) {
	try {
		std::string content = _extractFileContent(filePath);
		_parse(content);
		_validate();
	} catch(const std::exception& e) {
		throw std::runtime_error("Config: " + std::string(e.what()));
	}
}

Config::~Config() {}

std::string	Config::_extractFileContent(std::string const& filePath) {
	std::ifstream	file(filePath.c_str());
	if (!file.is_open())
		throw std::runtime_error("Could not open file: " + filePath);
	std::ostringstream	ss;
	ss << file.rdbuf();
	return ss.str();
}

void	Config::_parse(std::string const& content) {
	std::string token = "";
	std::vector<std::string> tokens;
	int braceDepth = 0;
	for (size_t i = 0; i < content.size(); ++i) {
		if (content[i] == '#') {
			_skipComment(content, i);
		} else if(content[i] == '{') {
			_parseBlock(tokens, content, i, braceDepth);
		} else if (isspace(content[i])) {
			_addTokenIf(token, tokens);
		} else {
			token += content[i];
		}
	}
}

void	Config::_parseBlock(std::vector<std::string>& tokens, std::string const& content,
	size_t& i, int& braceDepth) {
	if (tokens.size() <= 0)
		throw std::runtime_error("Unexpected '{'");
	++braceDepth;
	if (tokens[0] == "server" && tokens.size() == 1) {
		std::string blockContent = _getBlockContent(content, i, braceDepth);
		ServerBlock sb(blockContent);
		_servers.push_back(sb);
	if (tokens.size() <= 0)
		throw std::runtime_error("Unexpected '{'");
	} else {
		throw std::runtime_error("Invalid block: " + tokens[0]);
	}
	tokens.clear();
}

void	Config::_validate() const {
	if (_servers.empty())
		throw std::runtime_error("No server blocks defined");

	// Servers content validation
	std::vector<HostPortPair> allListen = utils::getAllListenPorts(_servers);
	if (!utils::hasVectorUniqEntries(allListen)) {
		throw std::runtime_error("Duplicate listen ip:port pairs over servers");
	}
	for (size_t i = 0; i < _servers.size(); ++i) {
		_servers[i].validate(); // validate this server block individually
	}
}

void	Config::_addTokenIf(std::string& token, std::vector<std::string>& tokens) {
	if (!token.empty()) {
		tokens.push_back(token);
		token.clear();
	}
}

std::string	Config::_getBlockContent(std::string const& content, size_t& index, int& braceDepth) {
	std::string	blockContent = "";
	index++; // skip the '{'
	while (index < content.size() && braceDepth > 0) {
		if (content[index] == '{') {
			++braceDepth;
		} else if (content[index] == '}') {
			--braceDepth;
			if (braceDepth == 0) {
				index++; // skip the '}'
				break;
			}
		}
		blockContent += content[index];
		++index;
	}
	if (braceDepth > 0)
		throw std::runtime_error("Unmatched '{' in configuration file");
	return blockContent;
}

void	Config::_skipComment(std::string const& content, size_t& index) {
	while (index < content.size() && content[index] != '\n')
		++index;
}

std::vector<ServerBlock> const&	Config::getServers() const {
	return _servers;
}

std::ostream&	operator<<(std::ostream& os, Config const& rhs) {
	std::vector<ServerBlock> servers = rhs.getServers();
	for (size_t i = 0; i < servers.size(); ++i) {
		os << servers[i];
	}
	return os;
}
