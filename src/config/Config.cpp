#include "config/Config.hpp"
#include "config/ServerBlock.hpp"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>

Config::Config(): _filePath("default.config") {}

Config::Config(std::string const& filePath): _filePath(filePath) {}


void	Config::parse() {
	std::string content = _extractFileContent();
	std::string token = "";
	std::vector<std::string> tokens;
	int braceDepth = 0;
	for (size_t i = 0; i < content.size(); ++i) {
		_skipCommentIf(content, i);
		if(content[i] == '{') {
			++braceDepth;
			if (tokens[0] == "server" && tokens.size() == 1) {
				ServerBlock sb;
				std::string blockContent = _getBlockContent(content, i, braceDepth);
				sb.parse(blockContent);
				_addServer(sb);
			if (tokens.size() <= 0)
				throw std::runtime_error("Unexpected '{'");
			} else {
				throw std::runtime_error("Invalid block: " + tokens[0]);
			}
			tokens.clear();
		} else if (isspace(content[i])) {
			_addTokenIf(token, tokens);
		} else {
			token += content[i];
		}
	}
}

std::string	Config::_extractFileContent() {
	std::ifstream	file(_filePath.c_str());
	if (!file.is_open())
		throw std::runtime_error("Could not open file: " + _filePath);
	std::ostringstream	ss;
	ss << file.rdbuf();
	return ss.str();
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

void	Config::_skipCommentIf(std::string const& content, size_t& index) {
	if (content[index] == '#') {
		while (index < content.size() && content[index] != '\n')
			++index;
	}
}

void	Config::_addServer(ServerBlock const& server) {
	_servers.push_back(server);
}

void	Config::print() const {
	for (size_t i = 0; i < _servers.size(); ++i) {
		_servers[i].print();
	}
}
