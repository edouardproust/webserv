#ifndef SERVER_BLOCK_HPP
#define SERVER_BLOCK_HPP

#include "config/LocationBlock.hpp"

class ServerBlock {

	std::string					_root;
	std::set<IpPortPair>		_listen;
	unsigned long				_clientMaxBodySize;
	std::map<int, std::string>	_errorPages;
	std::vector<std::string>	_indexFiles;
	std::vector<LocationBlock>	_locations;

	void		_parse(std::string const&);
	void		_parseBlock(std::vector<std::string>&, std::string const&, size_t&, int&, bool);
	void		_parseDirective(std::string&, std::vector<std::string>&, bool);
	IpPortPair	_parseIpPortPair(std::string const&);

	public:

		ServerBlock();
		ServerBlock(std::string const&);
		ServerBlock(ServerBlock const&);
		ServerBlock& operator=(ServerBlock const&);
		~ServerBlock();

		void					validate() const;
		LocationBlock const&	getBestLocationForPath(std::string const&);

		std::string const&					getRoot() const;
		std::set<IpPortPair> const&			getListen() const;
		unsigned long						getClientMaxBodySize() const;
		std::map<int, std::string> const&	getErrorPages() const;
		std::vector<std::string> const&		getIndexFiles() const;
		std::vector<LocationBlock> const&	getLocations() const;

};

std::ostream&	operator<<(std::ostream&, ServerBlock const&);

#endif
