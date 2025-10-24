#include "utils/utils.hpp"
#include <sstream>
#include <set>

template<typename T>
std::string	utils::toString(T const& value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

template<typename T>
bool	utils::hasVectorUniqEntries(std::vector<T> const& vec) {
	std::set<T> seen;
	for (typename std::vector<T>::const_iterator it = vec.begin(); it != vec.end(); ++it) {
		if (seen.find(*it) != seen.end()) {
			return false;
		}
		seen.insert(*it);
	}
	return true;
}

template<typename T, size_t N>
bool	utils::isInArray(T const& value, T const (&array)[N]) {
	for (size_t i = 0; i < N; ++i) {
		if (array[i] == value)
			return true;
	}
	return false;
}
