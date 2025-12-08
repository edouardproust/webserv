#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include "network/Client.hpp"
#include "network/Socket.hpp"
#include "http/RequestParser.hpp"
#include "router/Router.hpp"
#include "static/StaticHandler.hpp"
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
	static size_t const			_CLIENT_BUFFER_SIZE;
	static size_t const			_MAX_NB_OF_EVENTS;
	static size_t const			_MAX_READS_PER_CYCLE;
	static size_t const			_CLIENT_TIMEOUT_SECONDS;

	Config const&				_config;			// the parsed config file
	int							_epollFd;			// fd of the epoll instance
	std::vector<Socket*>		_listeningSockets;	// There is one Socket per listen directive in the config file
	std::vector<Client*>		_activeClients;
	CgiHandler					_cgi; 				// Instance of CgiHandler

	static std::string	_epollOpToString(int);
	static std::string	_epollEventToString(int);

	void	_initListeningSockets();
	void	_cleanupListeningSockets();

	void 	_registerNewClientToEpoll(Client*);
	void	_readClientRequest(Client*);
	void	_dispatchAndSendResponse(Client*);

	void	_createEpollInstance();
	void	_registerListeningSocketsToEpoll();
	int		_waitAndCollectEvents(struct epoll_event*, int);

	bool	_isListeningSocket(int);
	Client*	_acceptNewClient(int);
	void	_disconnectClient(int);
	void	_disconnectClient(Client*);

	void	_checkClientsInactivity();

	// Default and copy constructors, assignation are forbidden
	Network();
	Network(Network const&);
	Network&	operator=(Network const&);

	public:

		Network(Config const&);
		~Network();

		void	startServers();
		void	epollControl(int, int, uint32_t, const std::string&);

		int							getEpollFd() const;
		std::vector<Socket*> const&	getListeningSockets() const;
		std::vector<Client*> const&	getActiveClients() const;
		Client*						getClientByFd(int) const;
		bool						isClientConnected(int) const;
};

std::ostream&	operator<<(std::ostream&, Network const&);

#endif
