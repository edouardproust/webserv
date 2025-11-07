#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <cstring>
#include <iostream>

#include <network/Colors.hpp>
#include <iostream>
#include <config/Config.hpp>

	class Socket
	{
	private:
		struct addrinfo _hints;
		struct addrinfo *_servinfo;
		int _sock;
		HostPortPair _listen_on;

		void setAddrStruct();
		void loadAddressInfo();
		void createSocket();

	public:
		Socket(const HostPortPair &listen_pair);
		~Socket();
		void bind();
		void listen();
		int accept();

		const HostPortPair& getHostPortPair() const;
		int getSock();
		

		class GetAddrInfoException : public std::exception
		{
		public:
			const char *what() const throw();
		};
		class SocketException : public std::exception
		{
		public:
			const char *what() const throw();
		};
		class SetSockOptException : public std::exception
		{
		public:
			const char *what() const throw();
		};
		class FcntlException : public std::exception
		{
		public:
			const char *what() const throw();
		};
		class BindException : public std::exception
		{
		public:
			const char *what() const throw();
		};
		class ListenException : public std::exception
		{
		public:
			const char *what() const throw();
		};
		class AcceptException : public std::exception
		{
		public:
			const char *what() const throw();
		};
		class ConnectException : public std::exception
		{
		public:
			const char *what() const throw();
		};
	};

#endif 