#include "config/ServerBlock.hpp"

void	ServerBlock::parse(std::string& server_bloc) {
	/*for each line:
		if line starts with "location":
			parse LocationConfig
		else:
			parse server-level directives (root, server_name, listen)
	*/
}
