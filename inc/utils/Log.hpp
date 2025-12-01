#ifndef LOG_HPP
#define LOG_HPP

#include "utils/Const.hpp"
#include "utils/utils.hpp"
#include "utils/PrintableString.hpp"
#include <map>
#include <iomanip>
#include <ctime>

/**
 * Utility class for logging and formatting messages.
 *
 * Provides dev/prod logging streams, colored output, and string utilities.
 * Static service-type class: not instantiable or copyable.
 */
class Log
{
    static std::ostream&		_DEBUG_STREAM;
    static std::ostream&		_ACCESS_STREAM;
    static std::ostream&		_ERROR_STREAM;

    static std::string const	_RESET;
    static std::string const	_BOLD;
    static std::string const	_NORMAL;

    static std::string const	_RED;
    static std::string const	_BRIGHT_RED;
    static std::string const	_GREEN;
    static std::string const	_YELLOW;
    static std::string const	_BLUE;
    static std::string const	_MAGENTA;
    static std::string const	_CYAN;

	struct Category {
		std::string const	slug;
		std::string const	color;
		std::ostream&		stream;
	};

	static const Category	_CATEGORIES[];
	static const size_t		_CATEGORIES_SIZE;

	static Category const*	_findBySlug(std::string const&);

	template <typename T>
	static void	_print(const std::string&, const T&);


	// Not instantiable
	Log();
	Log(Log const&);
	Log&	operator=(Log const&);
	~Log();

	public:

		static size_t const	EXCERPT_CHARS;

		template <typename T>
		static void	dev(std::string const&, T const&);

		template <typename T>
		static void	prod(std::string const&, T const&);

		template <typename T>
		static std::string	hl(T const&);

		static std::string	excerpt(size_t n, std::string const& s);
};

#include "../src/utils/Log.tpp"

#endif