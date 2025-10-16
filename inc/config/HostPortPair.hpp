#ifndef HOST_PORT_PAIR_HPP
#define HOST_PORT_PAIR_HPP

#include <string>
#include <stdexcept>

class HostPortPair {

	std::string	_host;
	int			_port;

	static bool	_isValidIP(std::string const& ip);

	public:

		HostPortPair(const std::string& host, int port);

		const std::string&	getHost() const;
		int					getPort() const;

		bool	operator==(const HostPortPair& other) const;
		bool	operator<(const HostPortPair& other) const;

};

#endif