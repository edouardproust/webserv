#ifndef LOCATION_BLOCK_HPP
#define LOCATION_BLOCK_HPP

#include <string>
#include <vector>

class LocationBlock {

	std::string					_path;			// URI prefix, e.g. /cgi-bin/
	std::string					_root;			// optional, overrides server root
	std::string					_cgi_root;		// optional
	std::string					_cgi_extension;	// optional
	std::string					_cgi_executable;	// optional
	std::vector<std::string>	_index_files;	// optional

	LocationBlock();

	void	_addIndexFile(std::string const& index_file);

	public:

		LocationBlock(std::string const& path);

		void	parse(std::string&);

		std::string const&				getPath() const;
		std::string const&				getRoot() const;
		std::string const&				getCgiRoot() const;
		std::string const&				getCgiExtension() const;
		std::string const&				getCgiExecutable() const;
		std::vector<std::string> const&	getIndexFiles() const;

};

#endif
