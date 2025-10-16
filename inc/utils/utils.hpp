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
	bool	isAccessibleDirectory(std::string const& path); //TODO Unused yet but will be useful for static and cgi modules
	bool	isAbsolutePath(std::string const& path);

	int		toInt(std::string const&);

	std::string&	normalizePath(std::string& path);
	std::string		getFileExtension(std::string const& path);

}

#include "../src/utils/utils.tpp"

#endif