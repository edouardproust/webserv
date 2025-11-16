#ifndef HOST_PORT_PAIR_HPP
#define HOST_PORT_PAIR_HPP

#include "utils/utils.hpp"
#include <string>
#include <netdb.h>

/**
 * Value type class
 */
class HostPortPair {

	std::string	_host;
	size_t		_port;

	void	_parseHostPort(std::string const&);

	void	_setHost(std::string const&);
	void	_setPort(std::string const&);

	static bool	_isValidIP(std::string const&);

	public:

		HostPortPair();
		HostPortPair(std::string const&);
		HostPortPair(HostPortPair const&);
		HostPortPair& operator=(HostPortPair const&);
		~HostPortPair();

		const std::string&	getHost() const;
		size_t				getPort() const;

		bool	isWildcardFor(HostPortPair const& other) const;

		bool	operator==(const HostPortPair&) const;
		bool	operator<(const HostPortPair&) const;

};

std::ostream&	operator<<(std::ostream&, HostPortPair const&);

#endif
