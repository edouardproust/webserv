#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include "router/Router.hpp"
#include "network/Socket.hpp"
#include "utils/signal.hpp"
#include <sys/epoll.h>
#include <poll.h>

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
	std::map<int, std::string>	_pendingRequests;
	std::map<int, Socket*>		_clientServerMap;
	int							_epollFd;

	void 	_addClientToEpoll(int);
	void	_readClientRequest(int);
	void	_dispatchAndSendResponse(int);

	void	_epollCreate();
	void	_epollAddServers();
	void	_epollControl(int, int, uint32_t, const std::string&);
	int		_epollWait(struct epoll_event*);
	ssize_t	_safeRecv(int fd, void* buf, size_t size);
	ssize_t	_safeSend(int fd, const void* buf, size_t size);
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
		std::map<int, std::string> const&	getPendingRequests() const;
		std::map<int, Socket*> const&		getClientServerMap() const;
};

std::ostream&	operator<<(std::ostream&, Network const&);

#endif
