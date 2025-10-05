#ifndef SERVER_BLOCK_HPP
#define SERVER_BLOCK_HPP

#include "config/LocationBlock.hpp"
#include <string>
#include <vector>

class ServerBlock {

	std::string					_root;
	std::string					_server_name;
	int							_listen;
	std::vector<LocationBlock>	_locations;

	public:

		ServerBlock();

		void	parse(std::string const&);
		void	addLocation(LocationBlock const&);

		std::string const&					getRoot() const;
		std::string const&					getServerName() const;
		int									getListen() const;
		std::vector<LocationBlock> const&	getLocations() const;

};

#endif
