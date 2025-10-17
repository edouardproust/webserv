#ifndef SERVER_BLOCK_HPP
#define SERVER_BLOCK_HPP

#include "config/LocationBlock.hpp"
#include "config/HostPortPair.hpp"

class ServerBlock {

	std::string					_root;
	std::set<HostPortPair>		_listen;
	size_t						_clientMaxBodySize;
	bool						_isSetClientBodySize;
	std::map<int, std::string>	_errorPages;
	std::vector<std::string>	_indexFiles;

	std::vector<LocationBlock>	_locations;
	LocationBlock				_defaultLocation;

	void	_parse(std::string const&);
	void	_parseBlock(Tokens&, std::string const&, size_t&, int&, bool);
	void	_parseDirective(std::string&, Tokens&, bool);
	void	_addLocation(Tokens const&, std::string const&, size_t&, int&);

	void	_setRoot(Tokens const&);
	void	_setListen(Tokens const&);
	void	_setClientMaxBodySize(Tokens const&);
	void	_setErrorPages(Tokens const&);
	void	_setIndexFiles(Tokens const&);

	public:

		ServerBlock();
		ServerBlock(std::string const&);
		ServerBlock(ServerBlock const&);
		ServerBlock& operator=(ServerBlock const&);
		~ServerBlock();

		std::string const&					getRoot() const;
		std::set<HostPortPair> const&		getListen() const;
		size_t								getClientMaxBodySize() const;
		std::map<int, std::string> const&	getErrorPages() const;
		std::vector<std::string> const&		getIndexFiles() const;
		std::vector<LocationBlock> const&	getLocations() const;
		LocationBlock const&				getDefaultLocation() const;

};

std::ostream&	operator<<(std::ostream&, ServerBlock const&);

#endif
