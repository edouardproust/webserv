#ifndef SERVER_BLOCK_HPP
#define SERVER_BLOCK_HPP

#include "config/LocationBlock.hpp"
#include "config/HostPortPair.hpp"

class ServerBlock {

	std::vector<LocationBlock>	_locations;
	LocationBlock				_defaultLocation;
	std::string					_root;
	std::set<HostPortPair>		_listen;
	size_t						_clientMaxBodySize;
	bool						_isSetClientBodySize; // false if _clientBodySize is set on 0
	std::map<int, std::string>	_errorPages;
	std::vector<std::string>	_indexFiles;

	void			_parse(std::string const&);
	void			_parseBlock(Tokens&, std::string const&, size_t&, int&, bool);
	void			_parseDirective(std::string&, Tokens&, bool);
	HostPortPair	_parseHostPortPair(std::string const&);

	void	_setRoot(Tokens const&);
	void	_setListen(Tokens const&);
	void	_setClientMaxBodySize(Tokens const&);
	void	_setErrorPages(Tokens const&);
	void	_setIndexFiles(Tokens const&);
	void	_setLocations(Tokens const&, std::string const&, size_t&, int&);

	public:

		ServerBlock();
		ServerBlock(std::string const&);
		ServerBlock(ServerBlock const&);
		ServerBlock& operator=(ServerBlock const&);
		~ServerBlock();

		void	crossServersValidation() const;

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
