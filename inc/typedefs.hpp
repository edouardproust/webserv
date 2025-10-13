#ifndef TYPEDEFS_HPP
#define TYPEDEFS_HPP

#include <string>
#include <map>
#include <utility>

// http
typedef std::map<std::string, std::string>	Headers;		// {header_name:header_value, ...}

// config
typedef	std::pair<std::string, int>			HostPortPair;	// {IP:port}
typedef std::map<std::string, std::string>	CgiDirective;	// {extension:executable_path, ...}

#endif
