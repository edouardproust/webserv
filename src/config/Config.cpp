#include "config/Config.hpp"
#include "config/ServerBlock.hpp"
#include "config/LocationBlock.hpp"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>

Config::Config() {}

Config::Config(std::string const& filePath): _filePath(filePath) {}

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

std::string	Config::_getBlockContent(std::string const& content, size_t& index) {
	size_t	start = index;

	std::string	blockContent = "";
	while (index < content.size() && content[index] = '}') {
		blockContent += content[index];
		++index;
	}
	if (index >= content.size())
		throw std::runtime_error("Unmatched '{' in configuration file");
	return blockContent;
}

void	Config::parse() {
	std::string content = _extractFileContent();
	for (size_t i = 0; i < content.size(); ++i) {
		char c = content[i];
		std::string token = "";
		std::vector<std::string> tokens;
		if (c == '#') {
			while (i < content.size() && content[i] != '\n')
				++i;
		} else if (c == '{') {
			if (tokens.size() <= 0)
				throw std::runtime_error("Unexpected '{'");
			else if (tokens[0] == "server" && tokens.size() == 1) {
				ServerBlock sb;
				std::string blockContent = _getBlockContent(content, i);
				sb.parse(blockContent);
				_addServer(sb);
			} else if (tokens[0] == "location" && tokens.size() == 2) {
				if (_servers.empty())
					throw std::runtime_error("Location block outside of server block");
				LocationBlock lb(tokens[1]);
				std::string blockContent = _getBlockContent(content, i);
				lb.parse(blockContent);
				_servers.back().addLocation(lb);
			} else {
				throw std::runtime_error("Invalid block: " + tokens[0]);
			}
		} else if (c == '}') {
			_addTokenIf(token, tokens);
			// end of current block
			// pop context from stack
		} else if (c == ';') {
			_addTokenIf(token, tokens);
			// end of directive
			// add directive to current context}
		} else if (isspace(c)) {
			_addTokenIf(token, tokens);
		} else {
			token += c;
		}
	}
}

void	Config::_addServer(ServerBlock const& server) {
	_servers.push_back(server);
}

void	Config::print() const {
	for (size_t i = 0; i < _servers.size(); ++i) {
		std::cout << "Server " << i << ":\n";
		std::cout << "  Root: " << _servers[i].getRoot() << "\n";
		std::cout << "  Server Name: " << _servers[i].getServerName() << "\n";
		std::cout << "  Listen: " << _servers[i].getListen() << "\n";
		const std::vector<LocationBlock>& locations = _servers[i].getLocations();
		for (size_t j = 0; j < locations.size(); ++j) {
			std::cout << "  Location " << j << ":\n";
			std::cout << "    Path: " << locations[j].getPath() << "\n";
			std::cout << "    Root: " << locations[j].getRoot() << "\n";
			std::cout << "    CGI Root: " << locations[j].getCgiRoot() << "\n";
			std::cout << "    CGI Extension: " << locations[j].getCgiExtension() << "\n";
			std::cout << "    CGI Executable: " << locations[j].getCgiExecutable() << "\n";
			const std::vector<std::string>& indexFiles = locations[j].getIndexFiles();
			std::cout << "    Index Files: ";
			for (size_t k = 0; k < indexFiles.size(); ++k) {
				std::cout << indexFiles[k];
				if (k + 1 < indexFiles.size())
					std::cout << ", ";
			}
			std::cout << "\n";
		}
	}
}
