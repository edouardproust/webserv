#include "utils/utils.hpp"
#include <sstream>
#include <set>

template<typename T>
std::string	utils::toString(const T& value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

template<typename T>
bool utils::hasVectorUniqEntries(const std::vector<T> &vec) {
	std::set<T> seen;
	for (typename std::vector<T>::const_iterator it = vec.begin(); it != vec.end(); ++it) {
		if (seen.find(*it) != seen.end()) {
			return false;
		}
		seen.insert(*it);
	}
	return true;
}
