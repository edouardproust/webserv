#include "config/HostPortPair.hpp"
#include "utils/utils.hpp"
#include <netdb.h>

HostPortPair::HostPortPair(std::string const& host, int port)
	: _host(host), _port(port) {
	if (_port < 1 || _port > 65535) {
		throw std::out_of_range("Listen port out of range: " + utils::toString(_port));
	}
	if (_host == "localhost")
		_host = "127.0.0.1";
	if (!_isValidIP(_host))
		throw std::runtime_error("Invalid listen host IP address: " + _host);
}

/**
 * Check IPv4 and IPv6
 * "localhost" is the only non numeric value accepted for the IP address and is changed into "127.0.0.1"
 */
bool	HostPortPair::_isValidIP(std::string const& ip) {
    struct addrinfo hints = {};
    hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
    hints.ai_flags = AI_NUMERICHOST; // enforce numeric resolution
    struct addrinfo* res = NULL;
    int ret = getaddrinfo(ip.c_str(), NULL, &hints, &res);
    if (res) freeaddrinfo(res);
    return ret == 0;
}

std::string const&	HostPortPair::getHost() const {
	return _host;
}

int	HostPortPair::getPort() const {
	return _port;
}

bool	HostPortPair::operator==(HostPortPair const& other) const {
	return _host == other._host && _port == other._port;
}

bool	HostPortPair::operator<(HostPortPair const& other) const {
	if (_host != other._host) return _host < other._host;
	return _port < other._port;
}
