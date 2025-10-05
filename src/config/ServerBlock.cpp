#include "config/ServerBlock.hpp"

ServerBlock::ServerBlock() {}

void	ServerBlock::parse(std::string const& serverBloc) {
	(void)serverBloc;
}

void	ServerBlock::addLocation(LocationBlock const& location) {
	_locations.push_back(location);
}
