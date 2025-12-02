#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include "http/RequestParser.hpp"
#include "router/Router.hpp"
#include "network/Socket.hpp"
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

	Config const&				_config;			// the parsed config file
	int							_epollFd;			// fd of the epoll instance
	std::vector<Socket*>		_listeningSockets;	// There is one Socket per listen directive in the config file
	std::map<int, Socket*>		_socketsByClientFd; // Maps client fd with server socket
	std::map<int, Request>		_pendingRequests;	// Maps client fd with the Request object being built
	std::map<int, std::string>  _pendingResponses;	// Maps client fd with the raw response being built
	std::map<int, size_t>       _responseSendPos;	// Maps client fd with the cursor position in the raw response being built
	std::map<int, bool>         _shouldCloseAfterResponse; // Tells for each client fds, if the client socket must be closed after response was sent
	CgiHandler					_cgi; 				// Instance of CgiHandler

	void	_initListeningSockets();
	void	_cleanupListeningSockets();

	void 	_registerNewClientToEpoll(int);
	void	_readClientRequest(int);
	void	_dispatchAndSendResponse(int);
	void    _continuePendingSend(int clientFd);

	void	_createEpollInstance();
	void	_registerListeningSocketsToEpoll();
	int		_waitAndCollectEvents(struct epoll_event*);
	static std::string	_epollOpToString(int operation);

	bool	_isListeningSocket(int);
	int		_acceptNewClient(int);
	void	_prepareClientForNextRequest(int);
	void	_disconnectClient(int);

	// Default and copy constructors, assignation are forbidden
	Network();
	Network(Network const&);
	Network&	operator=(Network const&);

	public:

		Network(Config const&);
		~Network();

		void	startServers();
		void	epollControl(int, int, uint32_t, const std::string&);
		void	prepareResponseSend(int, Response const&);

		int									getEpollFd() const;
		std::vector<Socket*> const&			getListeningSockets() const;
		std::map<int, Request> const&		getPendingRequests() const;
		std::map<int, Socket*> const&		getSocketsByClientFd() const;
};

std::ostream&	operator<<(std::ostream&, Network const&);

#endif
