#include "config/Config.hpp"
#include "utils/utils.hpp"
#include "constants.hpp"
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
		if (content[i] == '#')
			skipComment(content, i);
		else if(content[i] == '{')
			_parseBlock(tokens, content, i, braceDepth);
		else if (isspace(content[i]))
			addTokenIf(token, tokens);
		else
			token += content[i];
	}
}

void	Config::_parseBlock(std::vector<std::string>& tokens, std::string const& content, size_t& i, int& braceDepth) {
	if (tokens.empty())
		throw std::runtime_error("Unexpected '{'");
	++braceDepth;
	std::string blockName = tokens[0];
	try {
		if (tokens[0] == "server")
			_addServer(tokens, content, i, braceDepth);
		// -- additional supported blocks can be added here --
		else
			throw std::runtime_error("Invalid block: " + tokens[0]);
	} catch (std::exception& e) {
		throw std::runtime_error(blockName + ": " + e.what()); // wrap error msg with the block name
	}
	tokens.clear();
}

/**
 * May throw an exception
 */
void	Config::_addServer(Tokens const& tokens, std::string const& content, size_t& i, int& braceDepth) {
	if (tokens.size() != 1)
		throw std::runtime_error("Invalid block declaration (should be 'server {...}')");
	std::string blockContent = getBlockContent(content, i, braceDepth);
	ServerBlock sb(blockContent);
	_servers.push_back(sb);
}

void	Config::_validate() const {
	if (_servers.empty())
		throw std::runtime_error("No server blocks defined");

	// Servers content validation
	std::vector<HostPortPair> allListen = getAllListenPorts();
	if (!utils::hasVectorUniqEntries(allListen)) {
		throw std::runtime_error("Duplicate listen ip:port pairs over servers");
	}
}

std::vector<HostPortPair>	Config::getAllListenPorts() const {
	std::vector<HostPortPair> all;
	for (size_t i = 0; i < _servers.size(); ++i) {
		std::set<HostPortPair> const& listen = _servers[i].getListen();
		all.insert(all.end(), listen.begin(), listen.end());
	}
	return all;
}

void	Config::addTokenIf(std::string& token, std::vector<std::string>& tokens) {
	if (!token.empty()) {
		tokens.push_back(token);
		token.clear();
	}
}

std::string	Config::getBlockContent(std::string const& content, size_t& index, int& braceDepth) {
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

void	Config::skipComment(std::string const& content, size_t& index) {
	while (index < content.size() && content[index] != '\n')
		++index;
}

/**
 * Accepts only size with maximum one unit char of [MmKkGg] at the end of the string,
 * and only non-float numerics:
 * - accepted: "100000", "10M", "1k", "5G"
 * - refused: "10MB", "10Kb", "0.5G", "3X"
 * - Also checks size_t overflow
 */
size_t	Config::parseSize(std::string const& sizeStr) {
	if (sizeStr.empty())
		throw std::runtime_error("Empty size string");

	unsigned char unit = sizeStr[sizeStr.size() - 1]; // unit is the last char of sizeStr
	size_t multiplier = 1;
	std::string numberStr = sizeStr;

	if (std::isalpha(unit)) {
		char u = std::tolower(unit);
		if (u == 'k') multiplier = 1024;
		else if (u == 'm') multiplier = 1024 * 1024;
		else if (u == 'g') multiplier = 1024 * 1024 * 1024;
		else
			throw std::runtime_error("Invalid unit: " + std::string(1, u));
		numberStr = sizeStr.substr(0, sizeStr.size() - 1);
		if (numberStr.empty())
			throw std::runtime_error("Missing numeric value before unit");
	}

	size_t number = utils::toSizeT(numberStr); // throw exception if empty, invalid number or overflow
	if (number > (MAX_SIZE_T / multiplier)) // this calculation is overflow-safe
		throw std::runtime_error("Too large number: "
			+ sizeStr + " (max is " + utils::toString(MAX_SIZE_T) + " bytes)");
	return number * multiplier;
}

std::vector<ServerBlock> const&	Config::getServers() const {
	return _servers;
}

std::ostream&	operator<<(std::ostream& os, Config const& rhs) {
	std::vector<ServerBlock> servers = rhs.getServers();
	for (size_t i = 0; i < servers.size(); ++i) {
		os << "Server " << i << ":\n" << servers[i];
	}
	return os;
}
