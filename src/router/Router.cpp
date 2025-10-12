#include "router/Router.hpp"
#include <iostream>

Router::Router(Request const& request, std::vector<ServerBlock> const& servers)
: _request(request), _servers(servers) {}

Router::~Router() {}

void	Router::resolve() const {
	// Match server block
	std::cout << "TODO: Resolve server block" << std::endl;
}

Request const&	Router::getRequest() const {
	return _request;
}

std::vector<ServerBlock> const&	Router::getServers() const {
	return _servers;
}

std::ostream&	operator<<(std::ostream& os, Router const& rhs) {
	Request const& request = rhs.getRequest();
	os << "Router:\n";
	os << "- Request method: " << request.getMethod() << "\n";
	os << "- Request path: " << request.getPath() << "\n";
	os << "- Request version: " << request.getVersion() << "\n";
	os << "- Headers: " << request.getHeaders().size() << std::endl;
	for (std::map<std::string, std::string>::const_iterator it = request.getHeaders().begin();
		it != request.getHeaders().end(); ++it) {
		os << "  - " << it->first << ": " << it->second << "\n";
	}
	return os;
}
