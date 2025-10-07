#ifndef SERVER_BLOCK_HPP
#define SERVER_BLOCK_HPP

#include "config/LocationBlock.hpp"
#include <string>
#include <vector>
#include <map>

class ServerBlock {

	std::string					_root;
	std::string					_serverName;
	unsigned int				_listen;
	unsigned long				_clientMaxBodySize;
	std::map<int, std::string>	_error_pages;
	std::vector<std::string>	_indexFiles;
	std::vector<LocationBlock>	_locations;

	ServerBlock();

	void	_parse(std::string const&);
	void	_parseBlock(std::vector<std::string>&, std::string const&, size_t&, int&, bool);
	void	_parseDirective(std::string& token, std::vector<std::string>&, bool);

	public:

		ServerBlock(std::string const&);

		void	validate();
		void	print() const;

		std::string const&					getRoot() const;
		std::string const&					getServerName() const;
		unsigned int						getListen() const;
		unsigned long						getClientMaxBodySize() const;
		std::map<int, std::string> const&	getErrorPages() const;
		std::vector<std::string> const&		getIndexFiles() const;
		std::vector<LocationBlock> const&	getLocations() const;

};

#endif
