#include "utils/Log.hpp"

const Log::Entry Log::TYPE_TABLE[] = {
	{"ok", BOLD GREEN, ACCESS_STREAM}, // Succeeded requests
	{"event", BLUE, ACCESS_STREAM}, // Request event
	{"status", CYAN, ACCESS_STREAM}, // Status, metrics
	{"info", BOLD YELLOW, ACCESS_STREAM}, // General acitivity informations

	{"error", BOLD RED, ERROR_STREAM}, // Critical errors
	{"warning", BOLD BRIGHT_RED, ERROR_STREAM},	// Warnings
	{"close", RED, ERROR_STREAM}, // Uncommon exits

	{"setup", MAGENTA, DEBUG_STREAM},
	{"debug", BLUE, DEBUG_STREAM},
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
