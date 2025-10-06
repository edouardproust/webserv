#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "config/ServerBlock.hpp"
#include <vector>

class Config {

	std::string					_filePath;
	std::vector<ServerBlock>	_servers;

	Config();

	void		_addServer(ServerBlock const&);
	std::string	_extractFileContent();

	public:

		Config(std::string const&);

		void	parse();
		void	print() const;

		static void			_addTokenIf(std::string&, std::vector<std::string>&);
		static std::string	_getBlockContent(std::string const&, size_t&, int&);
		static void			_skipCommentIf(std::string const&, size_t&);

		std::vector<ServerBlock> const&	getServers() const;
};

#endif
