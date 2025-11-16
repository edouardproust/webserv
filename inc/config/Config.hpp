#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "config/ServerBlock.hpp"
#include <fstream>

/**
 * Entity type class
 */
class Config {

	std::vector<ServerBlock>	_servers;

	std::string	_extractFileContent(std::string const&);
	void		_parse(std::string const&);
	void		_parseBlock(std::vector<std::string>&, std::string const&, size_t&, int&);
	void		_addServer(Tokens const&, std::string const&, size_t& i, int&);
	void		_validate() const;

	// Entity type - represents unique server configuration
    // Copying would duplicate expensive parsed data unnecessarily
	Config();
	Config(Config const&);
	Config&	operator=(Config const&);

	public:

		Config(std::string const&);
		~Config();

		std::vector<HostPortPair>	getAllListenPorts() const;

		static void			addTokenIf(std::string&, std::vector<std::string>&);
		static std::string	getBlockContent(std::string const&, size_t&, int&);
		static void			skipComment(std::string const&, size_t&);
		static size_t		parseSize(std::string const&);

		std::vector<ServerBlock> const&	getServers() const;

};

std::ostream&	operator<<(std::ostream&, Config const&);

#endif
