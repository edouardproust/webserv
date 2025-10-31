#ifndef UTILS_HPP
#define UTILS_HPP

#include "config/ServerBlock.hpp"
#include "constants.hpp"
#include <string>
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
	bool	isAccessibleDirectory(std::string const& path);
	bool	isReadableFile(std::string const& path);
	bool	isExecutableFile(std::string const& path);
	bool	fileExists(std::string const& path);

	size_t	getFileSize(const std::string& path);
	size_t	toSizeT(std::string const&);
	size_t	hexToSizeT(const std::string& hexStr);
	char	hexToChar(const std::string& hex);

	std::string		readFile(const std::string& path);
	std::string		getFileExtension(std::string const&);
	std::string		toLowerCase(const std::string&);
	std::vector<std::string>	split(std::string const&, char);
	std::string		joinPath(std::string const&, std::string const&);
	std::string		normalizePath(std::string const&);

}

#include "../src/utils/utils.tpp"

#endif
