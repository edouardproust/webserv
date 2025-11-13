#ifndef LOG_HPP
#define LOG_HPP

#include "constants.hpp"
#include "utils/utils.hpp"
#include "utils/PrintableString.hpp"
#include <map>
#include <iomanip>
#include <ctime>

class Log {

	#define DEBUG_STREAM	std::cerr
	#define ACCESS_STREAM	std::cout
	#define ERROR_STREAM	std::cerr

	#define RESET     	"\033[0m"
	#define BOLD		"\033[1m"
	#define NORMAL		"\033[0m"

	#define BLACK		"\033[30m"
	#define RED			"\033[31m"
	#define BRIGHT_RED	"\033[91m"
	#define GREEN		"\033[32m"
	#define YELLOW		"\033[33m"
	#define BLUE		"\033[34m"
	#define MAGENTA		"\033[35m"
	#define CYAN		"\033[36m"
	#define WHITE		"\033[37m"

	struct Entry { std::string const type; std::string const color; std::ostream& stream; };

	static const Entry TYPE_TABLE[];
	static const size_t TYPE_TABLE_SIZE;

	static Entry const*	_findByType(std::string const&);

	template <typename T>
	static void	_print(const std::string&, const T&);


	// TODO make canonical
	Log();
	Log(Log const&);
	Log&	operator=(Log const&);
	~Log();

	public:

		template <typename T>
		static void	dev(std::string const&, T const&);

		template <typename T>
		static void	prod(std::string const&, T const&);

		template <typename T>
		static std::string	hl(T const&);

};

#include "../src/utils/Log.tpp"

#endif