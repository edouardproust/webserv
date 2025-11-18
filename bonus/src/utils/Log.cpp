#include "utils/Log.hpp"

std::ostream&		Log::_DEBUG_STREAM	= std::cerr;
std::ostream&		Log::_ACCESS_STREAM	= std::cout;
std::ostream&		Log::_ERROR_STREAM	= std::cerr;

std::string const	Log::_RESET			= "\033[0m";
std::string const	Log::_BOLD			= "\033[1m";
std::string const	Log::_NORMAL		= "\033[0m";

std::string const	Log::_RED			= "\033[31m";
std::string const	Log::_BRIGHT_RED	= "\033[91m";
std::string const	Log::_GREEN			= "\033[32m";
std::string const	Log::_YELLOW		= "\033[33m";
std::string const	Log::_BLUE			= "\033[34m";
std::string const	Log::_MAGENTA		= "\033[35m";
std::string const	Log::_CYAN			= "\033[36m";

size_t const		Log::EXCERPT_CHARS	= 500;

const Log::Entry Log::TYPE_TABLE[] = {
	{"ok", _BOLD + _GREEN, _ACCESS_STREAM}, // Succeeded requests
	{"event", _BLUE, _ACCESS_STREAM}, // Request event
	{"status", _CYAN, _ACCESS_STREAM}, // Status, metrics
	{"info", _BOLD + _YELLOW, _ACCESS_STREAM}, // General acitivity informations

	{"error", _BOLD + _RED, _ERROR_STREAM}, // Critical errors
	{"warning", _BOLD + _BRIGHT_RED, _ERROR_STREAM},	// Warnings
	{"close", _RED, _ERROR_STREAM}, // Uncommon exits

	{"setup", _MAGENTA, _DEBUG_STREAM},
	{"debug", _BLUE, _DEBUG_STREAM},
};

const size_t Log::TYPE_TABLE_SIZE = sizeof(Log::TYPE_TABLE) / sizeof(Log::Entry);

Log::Entry const*	Log::_findByType(std::string const& type)
{
	for (size_t i = 0; i < TYPE_TABLE_SIZE; ++i)
		if (TYPE_TABLE[i].type == type)
			return &TYPE_TABLE[i];
	return NULL;
}

/**
 * Truncate a string as an excerpt, up to`n` chars.
 *
 * Will display `... (x bytes total)` after a truncated text.
 */
std::string	Log::excerpt(size_t n, std::string const& s)
{
	std::string excerpt = s.substr(0, n);
	if (s.size() > n)
    	excerpt += "... (" + utils::str(s.size()) + " bytes total)";
	return excerpt;
}
