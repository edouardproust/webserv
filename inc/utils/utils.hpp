#ifndef UTILS_HPP
#define UTILS_HPP

#include "constants.hpp"
#include <cstdlib>
#include <stdexcept>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
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
	bool	fileExists(std::string const&);

	size_t	getFileSize(const std::string&);
	size_t	toSizeT(std::string const&);
	size_t	hexToSizeT(const std::string&);
	char	hexToChar(const std::string&);

	std::string	readFile(const std::string&);
	std::string	getFileExtension(std::string const&);
	std::string	toLowerCase(std::string const&);
	std::vector<std::string>	split(std::string const&, char);

	std::string	joinPath(std::string const&, std::string const&);
	std::string	joinRelativePath(const std::string&);
	std::string	normalizePath(std::string const&);

	std::string	trim(const std::string& str);
	std::string	trimDomain(std::string const&);
	std::string	excerpt(size_t, std::string const&);

}

#include "../src/utils/utils.tpp"

#endif
