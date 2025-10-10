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

void	Router::resolve() {
	ServerBlock const* server = _matchServerBlock();
	if (server) {
		std::cout << "Matched server block:\n";
		server->print();
	} else {
		std::cout << "No matching server block found.\n";
	}
}

ServerBlock const*	Router::_matchServerBlock() const {
	IpPortPair const& reqHostPort = _getRequestHostPort();
	for (size_t i = 0; i < _servers.size(); ++i) {
		std::set<IpPortPair> const& listenSet = _servers[i].getListen();
		if (listenSet.find(reqHostPort) != listenSet.end()) {
			return &_servers[i];
		}
	}
	return NULL;
}

IpPortPair const&	Router::_getRequestHostPort() const {
	IpPortPair dummy = IpPortPair("localhost", 80);
	return (dummy);
}