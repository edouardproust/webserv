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
	bool		hasVectorUniqEntries(const std::vector<T> &);

	// others

	bool	isInt(std::string const&);
	bool	isAccessibleDirectory(std::string const&);
	bool	isExecutableFile(std::string const&);


	bool	isAbsolutePath(std::string const&);

	size_t	toSizeT(std::string const&);

	std::string		getFileExtension(std::string const&);
	std::string		toLowerCase(const std::string&);
	std::vector<std::string>	split(std::string const&, char);
	std::string		joinPath(std::string const&, std::string const&);
	std::string		normalizePath(std::string const&);

}

#include "../src/utils/utils.tpp"

#endif
