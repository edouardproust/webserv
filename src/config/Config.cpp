#include "Config.hpp"

void	Config::parse(std::string& file_path) {
/*
	try to open file

	foreach line:
		trim and split
		if line is emty or a comment: continue
		if token.first == "server" && token.last == "{":
			create new ServerBlock
			isInServer = true
		else if	isInServer && tokens.first == "location" && tokens.last == "{":
			create new LocationBlock
			isInLocation = true
		else if tokens.last = "}"
		else if ("}" is not the last !is_space char) or ("{" is the first !is_space char):
			throw error


		"server { ... }" block:
		create ServerConfig instance
		parse server block
		append to servers
	*/
}
