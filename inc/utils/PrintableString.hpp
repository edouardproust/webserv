#ifndef PRINTABLE_STRING_HPP
#define PRINTABLE_STRING_HPP

#include <iostream>
#include <sstream>
#include <string>

class PrintableString {

	const std::string& _ref;

	public:
		PrintableString(const std::string&);
		const std::string& get() const;

};

std::ostream&	operator<<(std::ostream& os, const PrintableString& p);
std::string operator+(const std::string& lhs, const PrintableString& rhs);
std::string operator+(const PrintableString& lhs, const std::string& rhs);

#endif
