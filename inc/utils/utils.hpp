#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <cstdlib>
#include <stdexcept>
#include <vector>

namespace utils {

	template<typename T>
	std::string	toString(const T&);

	template <typename T>
	bool	hasVectorUniqEntries(const std::vector<T> &vec);

	bool			isNumeric(std::string const&);
	bool			isAccessibleDirectory(std::string const& path); //TODO Unused but will be useful for static and cgi modules
	bool			isAbsolutePath(std::string const& path);

	unsigned long	parseSize(std::string const&);
	std::string&	normalizePath(std::string& path);
	std::string		toLowerCase(const std::string& str);

}

#include "../src/utils/utils.tpp"

#endif
