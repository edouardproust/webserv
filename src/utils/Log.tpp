#include "utils/Log.hpp"

template <typename T>
void	Log::dev(std::string const& category, T const& message)
{
	if (!DEVMODE)
		return;
	_print(category, message);
}

template <typename T>
void	Log::prod(std::string const& category, T const& message)
{
	_print(category, message);
}

/**
 * Turns any printable value into an highlighted string.
 */
template <typename T>
std::string	Log::hl(T const& value)
{
	std::string const& s = utils::str(value);
	std::string ret = s;
	if (DEVMODE)
		ret = _CYAN + s + _RESET;
	return ret;
}

template <typename T>
void Log::_print(std::string const& categorySlug, T const& value)
{
	Log::Category const* category = _findBySlug(categorySlug);
	if (category != NULL && category->slug == _DEBUG_SLUG && !PRINT_DEBUG)
		return;

	std::ostringstream oss;
	oss << value; // converts any printable type into a string stream

	std::string logLine = utils::formatDate(time(0), "%Y-%m-%d %H:%M:%S") + " ";
	if (category != NULL) {
		if (DEVMODE) // with color
			logLine += category->color + "[" + utils::toUpper(categorySlug) + "] " + _RESET + oss.str();
		else // without color
			logLine += "[" + utils::toUpper(categorySlug) + "] " + oss.str();
		category->stream << logLine << std::endl;
	} else { // not found (without color + error stream)
		logLine += "[" + utils::toUpper(categorySlug) + "] " + oss.str();
		_ERROR_STREAM << logLine << std::endl;

	}
}