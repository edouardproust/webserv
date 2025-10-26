#ifndef LOCATION_BLOCK_HPP
#define LOCATION_BLOCK_HPP

class ServerBlock;
#include "http/Request.hpp"
#include "utils/utils.hpp"
#include "typedefs.hpp"

class LocationBlock {

	ServerBlock const*			_server;				// Pointer to parent server block, set during validation
	std::string					_path;					// URI prefix (e.g. "/cgi-bin/")
	std::string					_root;					// optional, overrides server root
	std::string					_autoindex;				// optional
	std::set<std::string>		_limitExcept;			// optional (if empty, all methods are allowed)
	std::pair<int, std::string>	_return;				// optional (e.g. {"301", "/newpath/"})
	size_t						_clientMaxBodySize; 	// optional, overrides server limit.
	bool 						_isSetClientMaxBodySize;	// true if clientMaxBodySize is set in this location.
	std::string					_upload_store;			// TODO optional: uploads target directory (used if request method is POST and path is a static file)
	std::vector<std::string>	_indexFiles;			// optional, overrides server index files
	ErrorPages					_errorPages;			// optional. Has priority over _server->getErrorPages()
	CgiDirective				_cgi;					// optional (e.g. {".php": "/usr/bin/php-cgi", ".py": "/usr/bin/python"})

	LocationBlock(); // not used

	void	_parse(std::string const&);
	void	_parseDirective(std::string&, std::vector<std::string>&, bool);

	void	_setPath(std::string&);
	void	_setRoot(Tokens const&);
	void	_setAutoindex(Tokens const&);
	void	_setLimitExcept(Tokens const&);
	void	_setReturn(Tokens const&);
	void	_setClientMaxBodySize(Tokens const&);
	void	_setIndexFiles(Tokens const&);
	void	_setErrorPages(Tokens const&);
	void	_setCgi(Tokens const&);

	public:

		LocationBlock(ServerBlock const*); // Default location block (path = "/")
		LocationBlock(ServerBlock*, std::string&, std::string const&);
		LocationBlock(LocationBlock const&);
		LocationBlock&	operator=(LocationBlock const&);
		~LocationBlock();

		void setServer(ServerBlock* server);

		ServerBlock const*					getServer() const;
		std::string const&					getPath() const;
		std::string const					getRoot() const;
		std::string const					getAutoindex() const;
		std::set<std::string> const&		getLimitExcept() const;
		std::pair<int, std::string>	const&	getReturn() const;
		bool								getClientMaxBodySizeSet() const;
		size_t								getClientMaxBodySize() const;
		std::vector<std::string> const&		getIndexFiles() const;
		ErrorPages const&	getErrorPages() const;
		CgiDirective const&					getCgi() const;
		std::string const					getCgiExecutor(std::string const&) const;

		static LocationBlock const&			getDefaultLocation(ServerBlock const*);

		bool	isCgi(std::string const&) const;
		bool	isRedirection() const;
		bool	isAllowedMethod(std::string const&) const;
		bool	isAllowedClientBodySize(size_t, std::string const&) const;

};

std::ostream&	operator<<(std::ostream&, LocationBlock const&);

#endif
