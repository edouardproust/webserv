#ifndef UTILS_HPP
#define UTILS_HPP

#include "constants.hpp"
#include <cstdlib>
#include <stdexcept>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>

namespace utils {

	// templated

	template<typename T>
	std::string	toString(T const&);

	template<typename T>
	bool	hasVectorUniqEntries(std::vector<T> const&);

	template<typename T, size_t N>
	bool	isInArray(T const& value, T const (&array)[N]);

	// regular

	bool	isInt(std::string const&);
	bool	isAbsolutePath(std::string const&);
	bool	isAccessibleDirectory(std::string const&);
	bool	isReadableFile(std::string const&);
	bool	isExecutableFile(std::string const&);

	size_t	toSizeT(std::string const&);

	std::string		getFileExtension(std::string const&);
	std::string		toLowerCase(std::string const&);
	std::vector<std::string>	split(std::string const&, char);
	std::string		joinPath(std::string const&, std::string const&);
	std::string		normalizePath(std::string const&);
	std::string		trimDomain(std::string const&);

}

#include "../src/utils/utils.tpp"

#endif
