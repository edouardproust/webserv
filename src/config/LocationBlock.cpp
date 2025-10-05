#include "config/LocationBlock.hpp"

LocationBlock::LocationBlock();

LocationBlock::LocationBlock(std::string const& path): _path(path) {}

void	LocationBlock::parse(std::string& locationBloc) {
	(void)locationBloc;
}

void	LocationBlock::_addIndexFile(std::string const& index_file) {
	_index_files.push_back(index_file);
}
