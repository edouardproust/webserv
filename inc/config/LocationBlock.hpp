#ifndef LOCATION_BLOCK_HPP
#define LOCATION_BLOCK_HPP

#include <string>
#include <vector>

class LocationBlock {

	std::string					path;			// URI prefix, e.g. /cgi-bin/
	std::string					root;			// optional, overrides server root
	std::string					cgi_root;		// optional
	std::string					cgi_extension;	// optional
	std::string					cgi_executable;	// optional
	std::vector<std::string>	index_files;	// optional

	public:

		void	parse(std::string&);

};

#endif
