#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include "router/Router.hpp"
#include "network/Socket.hpp"
#include "utils/signal.hpp"
#include <sys/epoll.h>

/**
 * RAII wrapper managing server sockets, epoll, and client connections.
 *
 * Ensures proper acquisition and release of system resources.
 * Entity-type class: default and copy constructors are forbidden.
 */
class Network
{
	static size_t const			_CLIENT_BUFFER;
	static size_t const			_MAX_NB_OF_EVENTS;

	Config const&				_config;
	std::vector<Socket*>		_connections;
	std::map<int, std::string>	_requestList;
	std::map<int, Socket*>		_clientServerMap;
	int							_epollFd;

	void 	_addClientToEpoll(int);
	void	_recv(int);
	void	_send(int);
	bool	_isRequestComplete(const std::string&);

	void	_epollCreate();
	void	_epollAddServers();
	void	_epollControl(int, int, uint32_t, const std::string&);
	int		_epollWait(struct epoll_event*);
	static std::string	_epollOpToString(int operation);

	int		_acceptConnection(int);
	void	_manageConnection(int, bool);
	void	_handleClientDisconnect(int);

	// Default and copy constructors, assignation are forbidden
	Network();
	Network(Network const&);
	Network&	operator=(Network const&);

	public:

		Network(Config const&);
		~Network();

		void	startServers();

		int									getEpollFd() const;
		std::vector<Socket*> const&			getConnections() const;
		std::map<int, std::string> const&	getRequestList() const;
		std::map<int, Socket*> const&		getClientServerMap() const;

		class EpollException : public std::exception {
			public:
				char const*	what() const throw();
		};

		class EpollCtlException : public std::exception {
			public:
				char const*	what() const throw();
		};

		class EpollWaitException : public std::exception {
			public:
				char const*	what() const throw();
		};
};

std::ostream&	operator<<(std::ostream&, Network const&);

#endif
