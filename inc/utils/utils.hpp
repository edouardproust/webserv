#ifndef UTILS_HPP
#define UTILS_HPP

#include "config/ServerBlock.hpp"
#include <string>
#include <cstdlib>
#include <stdexcept>
#include <vector>

namespace utils {

	// templates

	template	<typename T>
	std::string	toString(const T&);

	template	<typename T>
	bool		hasVectorUniqEntries(const std::vector<T> &vec);

	// others

	bool	isInt(std::string const&);
	bool	isAccessibleDirectory(std::string const& path);
	bool	isExecutableFile(std::string const& path);


	bool	isAbsolutePath(std::string const& path);

	size_t	toSizeT(std::string const&);
	size_t	hexToSizeT(const std::string& hexStr);

	std::string&	normalizePath(std::string& path);
	std::string		getFileExtension(std::string const& path);
	std::string		toLowerCase(const std::string& str);

	char	hexToChar(const std::string& hex);
}

#include "../src/utils/utils.tpp"

#endif
