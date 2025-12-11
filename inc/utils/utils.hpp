#ifndef UTILS_HPP
#define UTILS_HPP

#include "utils/Const.hpp"
#include "utils/typedefs.hpp"
#include <cstdlib>
#include <stdexcept>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <cerrno>
#include <map>

namespace utils {

	// templated

	template <typename T>
	std::string	str(T const&);

	template<typename T>
	bool	hasVectorUniqEntries(std::vector<T> const&);

	template<typename T, size_t N>
	bool	isInArray(T const&, T const (&)[N]);

	// regular

	bool	isInt(std::string const&);
	bool	isAbsolutePath(std::string const&);
	bool	isRelativePath(std::string const&);

	bool	isAccessibleDirectory(std::string const&);
	bool	isWritableDirectory(std::string const&);
	bool	isReadableFile(std::string const&);
	bool	isExecutableFile(std::string const&);
	bool	fileExists(std::string const&);

	size_t	getFileSize(std::string const&);
	size_t	toSizeT(std::string const&);
	size_t	hexToSizeT(std::string const&);
	char	hexToChar(std::string const&);
	std::string	toUpper(std::string const& s);

	std::string	readFile(std::string const&);
	std::string	getFileName(std::string const&);
	std::string	getFileExtension(std::string const&);
	std::string	toLowerCase(std::string const&);
	std::vector<std::string>	split(std::string const&, char);

	std::string	pathsJoin(std::string const&, std::string const&);
	std::string securedPathsJoin(std::string const&, std::string const&);
	std::string	normalizePath(std::string const&);
	std::string	trim(std::string const&);
	std::string	formatDate(time_t, const std::string&);
	std::string extractHostname(UniqHeaders const&);
	std::pair<size_t, size_t>	headersBodySeparatorPos(std::string const&, bool = false);
	std::string	getParentDirectory(const std::string&);

}

#include "../src/utils/utils.tpp"

#endif