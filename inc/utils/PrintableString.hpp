#ifndef PRINTABLE_STRING_HPP
#define PRINTABLE_STRING_HPP

#include <iostream>
#include <sstream>
#include <string>

#define FT_EMPTY "[empty]"

/**
 * Wrapper around std::string for safe logging/printing.
 *
 * Encapsulates a string value and provides consistent output formatting.
 * Value-type class: copyable and assignable; follows orthodox canonical form.
 */
class PrintableString
{
	std::string	_str;

	public:

		// Orthodox canonical form
		PrintableString();
		PrintableString(std::string const&);
		PrintableString(PrintableString const&);
		PrintableString& operator=(PrintableString const&);
		~PrintableString();

		std::string const& get() const;
};

std::ostream&	operator<<(std::ostream&, PrintableString const&);
std::string		operator+(std::string const&, PrintableString const&);
std::string		operator+(PrintableString const&, std::string const&);

#endif
