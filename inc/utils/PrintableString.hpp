#ifndef PRINTABLE_STRING_HPP
#define PRINTABLE_STRING_HPP

#include <iostream>
#include <sstream>
#include <string>

#define FT_EMPTY "[empty]"

/**
 * Orthodox canonical class (Value-type)
 */
class PrintableString {

	std::string	_str;

	public:
		PrintableString();
		PrintableString(std::string const&);

		PrintableString(PrintableString const&);
		PrintableString& operator=(PrintableString const&);
		~PrintableString();
		std::string const& get() const;

};

std::ostream&	operator<<(std::ostream& os, const PrintableString& p);
std::string operator+(const std::string& lhs, const PrintableString& rhs);
std::string operator+(const PrintableString& lhs, const std::string& rhs);

#endif
