#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "config/ServerBlock.hpp"
#include <vector>

class Config {

	std::vector<ServerBlock>	_servers;

	std::string	_extractFileContent(std::string const&);
	void		_parse(std::string const&);
	void		_parseBlock(std::vector<std::string>&, std::string const& content,
					size_t&, int&);
	void		_validate() const;

	// Not implemented (not used)
	Config();
	Config(Config const&);
	Config&	operator=(Config const&);

	public:

		static unsigned long	MAX_CLIENT_BODY_SIZE;

		Config(std::string const&);
		~Config();

		void	print() const;

		static void			_addTokenIf(std::string&, std::vector<std::string>&);
		static std::string	_getBlockContent(std::string const&, size_t&, int&);
		static void			_skipComment(std::string const&, size_t&);

		std::vector<ServerBlock> const&	getServers() const;
};

#endif
