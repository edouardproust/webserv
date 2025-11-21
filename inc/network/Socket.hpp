#ifndef SOCKET_HPP
#define SOCKET_HPP

#include "config/Config.hpp"
#include <fcntl.h>
#include <cstring>

/**
 * RAII wrapper for a system socket.
 *
 * Manages creation, binding, listening, and accepting connections.
 * Resource-type class: not copyable or assignable; enforces RAII ownership.
 */
class Socket
{
	struct addrinfo		_hints;
	struct addrinfo*	_servinfo;
	int					_fd;
	HostPortPair		_listenOn;

	void	_setAddrStruct();
	void	_loadAddressInfo();
	void	_createSocket();

	// Default and copy constructors, assignation are forbidden
	Socket();
	Socket(Socket const&);
	Socket&	operator=(Socket const&);

	public:

		Socket(HostPortPair const&);
		~Socket();

		void	bind();
		void	listen();
		int		accept();

		HostPortPair const&	getHostPortPair() const;
		int					getFd() const;

};

#endif