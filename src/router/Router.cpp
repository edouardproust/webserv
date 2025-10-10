#include "router/Router.hpp"
#include <iostream>

Router::Router(Request const& request, std::vector<ServerBlock> const& servers)
: _request(request), _servers(servers) {}

Router::~Router() {}

void	Router::print() {
	std::cout << "Router:\n"
		<< "- Request method: " << _request.getMethod() << "\n"
		<< "- Request path: " << _request.getPath() << "\n"
		<< "- Request version: " << _request.getVersion() << "\n"
		<< "- Headers: " << _request.getHeaders().size() << std::endl;
		for (std::map<std::string, std::string>::const_iterator it = _request.getHeaders().begin();
			it != _request.getHeaders().end(); ++it) {
			std::cout << "  - " << it->first << ": " << it->second << "\n";
		}
		std::cout << std::endl;
}