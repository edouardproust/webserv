#include "utils/PrintableString.hpp"

PrintableString::PrintableString(const std::string& s) : _ref(s) {}

PrintableString::~PrintableString() {}

std::string const&	PrintableString::get() const {
	return _ref;
}

/**
 * Changes the display rules of a PrintableString
 */
std::ostream&	operator<<(std::ostream& os, const PrintableString& p) {
	os << (!p.get().empty() ? "\"" + p.get() + "\"" : FT_EMPTY );

	return os;
}

std::string operator+(const std::string& lhs, const PrintableString& rhs) {
    std::ostringstream oss;
    oss << lhs << rhs;
    return oss.str();
}

std::string operator+(const PrintableString& lhs, const std::string& rhs) {
    std::ostringstream oss;
    oss << lhs << rhs;
    return oss.str();
}
