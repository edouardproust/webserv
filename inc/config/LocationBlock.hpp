#ifndef LOCATION_BLOCK_HPP
#define LOCATION_BLOCK_HPP

#include "config/ServerBlock.hpp"
#include <string>
#include <vector>
#include <set>
#include <utility>

class LocationBlock: public ServerBlock {

	std::string					_path;				// URI prefix, e.g. /cgi-bin/
	std::string					_root;				// optional, overrides server root
	std::string					_autoindex;			// optional
	std::set<std::string>		_limitExcept;		// optional (if empty, all methods are allowed)
	std::pair<int, std::string>	_return;			// optional (e.g. 301 /newpath/)
	std::string					_clientMaxBodySize; // inherited from server if not set
	std::string					_cgiRoot;			// optional
	std::string					_cgiExtension;		// optional
	std::string					_cgiExecutable;		// optional
	std::vector<std::string>	_indexFiles;		// optional

	LocationBlock();

	public:

		LocationBlock(std::string const&);

		void	parse(std::string&);
		void	print() const;

		std::string const&					getPath() const;
		std::string const&					getRoot() const;
		std::string const&					getAutoindex() const;
		std::set<std::string> const&		getAllowedMethods() const;
		std::pair<int, std::string>	const&	getReturn() const;
		std::string const&					getClientMaxBodySize() const;
		std::string const&					getCgiRoot() const;
		std::string const&					getCgiExtension() const;
		std::string const&					getCgiExecutable() const;
		std::vector<std::string> const&		getIndexFiles() const;

};

#endif
