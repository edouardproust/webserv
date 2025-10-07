#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <cstdlib>
#include <stdexcept>

namespace utils {

	template<typename T>
	std::string		to_string(const T&);

	bool			isNumeric(std::string const&);
	unsigned long	parseSize(std::string const&);

}

#include "../src/utils/utils.tpp"

#endif