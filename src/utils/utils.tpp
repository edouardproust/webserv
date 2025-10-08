#include "utils/utils.hpp"
#include <sstream>

template<typename T>
std::string	utils::toString(const T& value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}
