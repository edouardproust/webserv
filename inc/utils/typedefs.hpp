#ifndef TYPEDEFS_HPP
#define TYPEDEFS_HPP

#include <string>
#include <map>
#include <vector>
#include <utility>

// http

typedef std::vector<std::pair<std::string, std::string> >	AllHeaders; // Allows duplicate entries
typedef std::map<std::string, std::string>					UniqHeaders; // Unique entries {header_name:header_value, ...}

// config

typedef std::map<std::string, std::string>	CgiDirective; // {extension:executable_path, ...}
typedef std::vector<std::string>			Tokens;
typedef std::map<int, std::string>			ErrorPages;

#endif
