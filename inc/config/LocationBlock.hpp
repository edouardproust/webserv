#ifndef LOCATION_BLOCK_HPP
#define LOCATION_BLOCK_HPP

class ServerBlock;
#include "typedefs.hpp"
#include <set>

class LocationBlock {

	ServerBlock*				_server;				// Pointer to parent server block, set during validation
	std::string					_path;					// URI prefix (e.g. "/cgi-bin/")
	std::string					_root;					// optional, overrides server root
	std::string					_autoindex;				// optional
	std::set<std::string>		_limitExcept;			// optional (if empty, all methods are allowed)
	std::pair<int, std::string>	_return;				// optional (e.g. {"301", "/newpath/"})
	unsigned long				_clientMaxBodySize; 	// optional, overrides server limit
	bool 						_clientMaxBodySizeSet;  // true if clientMaxBodySize is set in this location
	std::vector<std::string>	_indexFiles;			// optional, overrides server index files
	CgiDirective				_cgi;					// optional (e.g. {".php": "/usr/bin/php-cgi", ".py": "/usr/bin/python"})

	void	_parse(std::string const&);
	void	_parseDirective(std::string& token, std::vector<std::string>&, bool);
	void	_addCgiDirective(std::string extension, std::string executable);

	LocationBlock(); // not used

	public:

		LocationBlock(ServerBlock*); // Default location block (path = "/")
		LocationBlock(ServerBlock*, std::string const&, std::string const&);
		LocationBlock(LocationBlock const&);
		LocationBlock&	operator=(LocationBlock const&);
		~LocationBlock();

		void	validate() const;
		bool	isCgiLocation() const;
		bool	isRedirectionLocation() const;

		ServerBlock*						getServer() const;
		std::string const&					getPath() const;
		std::string const					getRoot() const;
		std::string const&					getAutoindex() const;
		std::set<std::string> const&		getLimitExcept() const;
		std::pair<int, std::string>	const&	getReturn() const;
		bool								getClientMaxBodySizeSet() const;
		unsigned long						getClientMaxBodySize() const;
		std::vector<std::string> const&		getIndexFiles() const;
		CgiDirective const&					getCgi() const;
		std::string const					getCgiExecutor(std::string const& extension) const;

		void setServer(ServerBlock* server);
		void setReturn(Tokens const& tokens);

};

std::ostream&	operator<<(std::ostream&, LocationBlock const&);

#endif
