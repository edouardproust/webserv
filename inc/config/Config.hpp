#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "ServerBlock.hpp"
#include <vector>

class Config {

	std::vector<ServerBlock>   servers;

	public:

		void	parse(std::string&);
};

#endif
