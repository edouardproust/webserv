#ifndef SERVER_BLOCK_HPP
#define SERVER_BLOCK_HPP

#include "LocationBlock.hpp"
#include <string>
#include <vector>

class ServerBlock {

	std::string					root;
	std::string					server_name;
	int							listen;
	std::vector<LocationBlock>	locations;

	public:

		void	parse(std::string&);

};

#endif
