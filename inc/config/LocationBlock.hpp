#ifndef LOCATION_BLOCK_HPP
#define LOCATION_BLOCK_HPP

class ServerBlock;
#include <string>
#include <vector>
#include <set>
#include <utility>

class LocationBlock {

	std::string					_path;					// URI prefix, e.g. /cgi-bin/
	std::string					_root;					// optional, overrides server root
	std::string					_autoindex;				// optional
	std::set<std::string>		_limitExcept;			// optional (if empty, all methods are allowed)
	std::pair<int, std::string>	_return;				// optional (e.g. 301 /newpath/)
	unsigned long				_clientMaxBodySize; 	// optional, overrides server limit
	bool 						_clientMaxBodySizeSet;  // true if clientMaxBodySize is set in this location
	std::vector<std::string>	_indexFiles;			// optional, overrides server index files
	std::string					_cgiRoot;				// optional
	std::string					_cgiExtension;			// optional
	std::string					_cgiExecutable;			// optional

	void	_parse(std::string const&);
	void	_parseDirective(std::string& token, std::vector<std::string>&, bool);

	LocationBlock(); // Not used

	public:

		LocationBlock(std::string const&, std::string const&);
		LocationBlock(LocationBlock const&);
		LocationBlock&	operator=(LocationBlock const&);
		~LocationBlock();

		void	validate(ServerBlock const&) const;
		void	print() const;

		std::string const&					getPath() const;
		std::string const					getRoot(ServerBlock const&) const;
		std::string const&					getAutoindex() const;
		std::set<std::string> const&		getAllowedMethods() const;
		std::pair<int, std::string>	const&	getReturn() const;
		unsigned long						getClientMaxBodySize(ServerBlock const&) const;
		std::string const&					getCgiRoot() const;
		std::string const&					getCgiExtension() const;
		std::string const&					getCgiExecutable() const;
		std::vector<std::string> const&		getIndexFiles(ServerBlock const&) const;

};

#endif
