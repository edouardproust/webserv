#include "config/HostPortPair.hpp"
#include "utils/utils.hpp"
#include <netdb.h>

/**
 * May throw an exception
 */
HostPortPair::HostPortPair(std::string const& hostPortStr) {
	_parseHostPort(hostPortStr); // throw
}

HostPortPair::HostPortPair(HostPortPair const& other) : _host(other._host), _port(other._port)
{}

HostPortPair& HostPortPair::operator=(HostPortPair const& other) {
	_host = other._host;
	_port = other._port;
	return *this;
}

HostPortPair::~HostPortPair() {}

/**
 * May throw an exception
 */
void	HostPortPair::_parseHostPort(std::string const& hostPortStr) {
	std::string::size_type colonPos = hostPortStr.rfind(':');
	if (colonPos != std::string::npos) {
		// Case: "host:port"
		_setHost(hostPortStr.substr(0, colonPos)); // throw
		_setPort(hostPortStr.substr(colonPos + 1)); // throw
	} else {
		// Case: "host" or "port"
		if (utils::isInt(hostPortStr)) {
			_setPort(hostPortStr); // throw
			_setHost("0.0.0.0"); // by default, listen on all IPs | throw
		} else {
			_setPort("80"); // default port is 80 | throw
			_setHost(hostPortStr); // throw
		}
	}
}

/**
 * May throw an exception
 */
void	HostPortPair::_setHost(std::string const& hostStr) {
	if (hostStr.empty())
		throw std::runtime_error("A host is empty");
	std::string host = hostStr;

	// Resolve host names
	if (host == "localhost") host = "127.0.0.1";
	// -- additional supported host names can be added here --

	if (!_isValidIP(host))
		throw std::runtime_error("Invalid listen host IP address: " + host);
	_host = host;
}

/**
 * May throw an exception
 */
void	HostPortPair::_setPort(std::string const& portStr) {
	_port = utils::toSizeT(portStr); // throw if empty, invalid number or int overflow
	if (_port < 1 || _port > 65535) {
		throw std::out_of_range("Port out of range: " + portStr);
	}
}

/**
 * Check IPv4 only
 * //TODO implement support of IPv6 ?
 * "localhost" is the only non numeric value accepted for the IP address and is changed into "127.0.0.1"
 */
bool	HostPortPair::_isValidIP(std::string const& ip) {
    struct addrinfo hints = {};
    hints.ai_family = AF_INET; // IPv4 only (would be AF_UNSPEC to also support IPv6)
    hints.ai_flags = AI_NUMERICHOST; // enforce numeric resolution
    struct addrinfo* res = NULL;
    int ret = getaddrinfo(ip.c_str(), NULL, &hints, &res);
    if (res) freeaddrinfo(res);
    return ret == 0;
}

std::string const&	HostPortPair::getHost() const {
	return _host;
}

size_t	HostPortPair::getPort() const {
	return _port;
}

bool	HostPortPair::operator==(HostPortPair const& other) const {
	return _host == other._host && _port == other._port;
}

bool	HostPortPair::operator<(HostPortPair const& other) const {
	if (_host != other._host) return _host < other._host;
	return _port < other._port;
}
