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
	int					_sock;
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
		int					getSock();

		class GetAddrInfoException : public std::exception {
			public:
				char const*	what() const throw();
		};

		class SocketException : public std::exception {
			public:
				char const*	what() const throw();
		};

		class SetSockOptException : public std::exception {
			public:
				char const*	what() const throw();
		};

		class FcntlException : public std::exception {
			public:
				char const*	what() const throw();
		};

		class BindException : public std::exception {
			public:
				char const*	what() const throw();
		};

		class ListenException : public std::exception {
			public:
				char const*	what() const throw();
		};

		class AcceptException : public std::exception {
			public:
				char const*	what() const throw();
		};

		class ConnectException : public std::exception {
			public:
				char const*	what() const throw();
		};
};

#endif