#include "utils/Log.hpp"

template <typename T>
void	Log::dev(std::string const& type, T const& message) {
	if (!DEVMODE)
		return;
	_print(type, message);
}

template <typename T>
void	Log::prod(std::string const& type, T const& message) {
	_print(type, message);
}

/**
 * Turns any printable value into an highlighted string.
 */
template <typename T>
std::string	Log::hl(T const& value) {
	std::string const& s = utils::str(value);
	std::string ret = s;
	if (DEVMODE)
		ret = CYAN + s + RESET;
	return ret;
}

template <typename T>
void Log::_print(std::string const& type, T const& value) {
	std::string logLine = utils::getCurrentDate("%Y-%m-%d %H:%M:%S") + " ";
	Log::Entry const* res = _findByType(type);

	std::ostringstream oss;
	oss << value; // converts any printable type into a string stream

	if (res != NULL) {
		if (DEVMODE)
			logLine += res->color + "[" + utils::toUpper(type) + "]" + RESET + " " + oss.str();
		else
			logLine += "[" + utils::toUpper(type) + "]" + " " + oss.str();
	} else { // not found
		logLine += "[" + type + "] " + oss.str();
	}

	res->stream << logLine << std::endl;
}