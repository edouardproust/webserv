#ifndef SERVER_BLOCK_HPP
#define SERVER_BLOCK_HPP

#include "config/LocationBlock.hpp"
#include <string>
#include <vector>
#include <map>

class ServerBlock {

	std::string					_root;
	std::string					_serverName;
	int							_listen;
	std::map<int, std::string>	_error_pages;
	std::vector<std::string>	_indexFiles;
	std::vector<LocationBlock>	_locations;

	void	_addLocation(LocationBlock const&);

	protected:

		std::string					_clientMaxBodySize;

	public:

		ServerBlock();

		void	parse(std::string const&);
		void	print() const;

		std::string const&					getRoot() const;
		std::string const&					getServerName() const;
		int									getListen() const;
		std::string const&					getClientMaxBodySize() const;
		std::map<int, std::string> const&	getErrorPages() const;
		std::vector<std::string> const&		getIndexFiles() const;
		std::vector<LocationBlock> const&	getLocations() const;

};

#endif
