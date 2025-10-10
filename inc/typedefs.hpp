#ifndef TYPEDEFS_HPP
#define TYPEDEFS_HPP

#include <string>
#include <map>
#include <utility>

// config
typedef	std::pair<std::string, int>			IpPortPair;	// {IP:port}
typedef std::map<std::string, std::string>	CgiDirective;		// {extension:executable_path, ...}

#endif
