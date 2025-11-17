#include "utils/PrintableString.hpp"

PrintableString::PrintableString()
: _str("")
{}

PrintableString::PrintableString(std::string const& str)
: _str(str)
{}

PrintableString::PrintableString(PrintableString const& other)
: _str(other._str)
{}

PrintableString&	PrintableString::operator=(PrintableString const& other)
{
	if (this != &other) {
		_str = other._str;
	}
	return *this;
}

PrintableString::~PrintableString()
{}

std::string const&	PrintableString::get() const
{
	return _str;
}

/**
 * Changes the display rules of a PrintableString
 */
std::ostream&	operator<<(std::ostream& os, PrintableString const& p)
{
	os << (!p.get().empty() ? "\"" + p.get() + "\"" : FT_EMPTY );
	return os;
}

std::string operator+(std::string const& lhs, PrintableString const& rhs)
{
    std::ostringstream oss;
    oss << lhs << rhs;
    return oss.str();
}

std::string operator+(PrintableString const& lhs, std::string const& rhs)
{
    std::ostringstream oss;
    oss << lhs << rhs;
    return oss.str();
}
