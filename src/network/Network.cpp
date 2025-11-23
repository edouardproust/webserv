#include "network/Network.hpp"

size_t const	Network::_CLIENT_BUFFER_SIZE = 1024 * 1024; // 1MB
size_t const	Network::_MAX_NB_OF_EVENTS = 100;

Network::Network(Config const& config)
: _config(config)
, _epollFd(-1)
{
	// Get all unique host:port pairs directly from config
	std::vector<HostPortPair> listenPorts = _config.getAllListenPorts();

	// Create socket for each HostPortPair
	for (size_t i = 0; i < listenPorts.size() && sig::keepRunning(); ++i)
		_listeningSockets.push_back(new Socket(listenPorts[i]));

	//  Bind and listen to each socket created
	for (size_t i = 0; i < _listeningSockets.size(); ++i) {
		_listeningSockets[i]->bind();
		_listeningSockets[i]->listen();
	}
}

Network::~Network()
{
	Log::dev("close", "Closing Web server...");

	Log::dev("close", "Freeing Socket memory.");
	for (size_t i = 0; i < _listeningSockets.size(); ++i)
		delete _listeningSockets[i];

	Log::dev("close", "Closing epoll...");
	if (_epollFd != -1)
		close(_epollFd);

	Log::prod("ok", Const::SERVER_NAME + " closed.");
	std::cout << std::endl;
}


// MAIN LOGIC

/**
 * @note `eventFd` can be the file descriptor of:
 * a listening socket (new client connexion),
 * an existing client (data to read/write),
 * or a CGI Pipe (output)
 */
void Network::startServers()
{
	Log::prod("ok", Const::SERVER_NAME + " started.");
	Log::prod("info", "Press Ctrl + C to stop the Web server.");

	_startEpoll(); // create the fd for epoll instance
	_addListeningSocketsToEpoll(); // add listening sockets to the surveillance

	struct epoll_event events[_MAX_NB_OF_EVENTS];
	while (sig::keepRunning()) {
		int readyEventsCount = _waitAndCollectEvents(events);
		for (int n = 0; n < readyEventsCount && sig::keepRunning(); n++) {
			int eventFd = events[n].data.fd;
			if (_isListeningSocket(eventFd)) { // 1. Event on listening socket = new client
				int newClientFd = _acceptNewClient(eventFd);
				if (newClientFd > 0) {
					_addClientToEpoll(newClientFd);
				} else {
					Log::prod("error", "Accept failed on listening socket.");
				}
			} else { // 2. Existing client fd or CGI pipe
				uint32_t ev = events[n].events;
				if (ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) // error
					_disconnectClient(eventFd);
				//else if (_isCgiEvent(eventFd)) // eventFd is a CGI pipe fd // TODO
            	//	_handleCgiOutput(eventFd); // TODO
        		else if (ev & EPOLLIN) { // can receive from client (eventFd is an existing client fd)
					_readClientRequest(eventFd);
				} else if (ev & EPOLLOUT) { // can send to client (eventFd is an existing client fd)
					if (_pendingResponses.find(eventFd) != _pendingResponses.end())
						_continuePendingSend(eventFd);
					else
						_dispatchAndSendResponse(eventFd); // (also prepares CGI pipes) // TODO
				}
			}
		}
	}
}

void	Network::_addClientToEpoll(int clientFd)
{
	// Configuration of client's socket
	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1
	|| fcntl(clientFd, F_SETFD, FD_CLOEXEC) == -1) {
		Log::prod("error", "fcntl failed for on fd " + utils::str(clientFd) + ": " + strerror(errno));
		close(clientFd);
		return;
	}
	// Add to epoll
	_epollControl(clientFd, EPOLL_CTL_ADD, EPOLLIN, "new client setup"); // throw
	Log::dev("event", "New client fd " + Log::hl(clientFd) + " configured and added to epoll");
}

void Network::_readClientRequest(int clientFd)
{
	char buff[_CLIENT_BUFFER_SIZE];
	std::string& currentReq = _pendingRequests[clientFd];
	int bytesReceived = recv(clientFd, buff, _CLIENT_BUFFER_SIZE, 0);
	if (bytesReceived > 0) {
		if (currentReq.size() + bytesReceived > Const::ABSOLUTE_MAX_CLIENT_BODY_SIZE) {
            Log::prod("error", "413 Payload Too Large from fd " + Log::hl(clientFd) + " (" + utils::str(currentReq.size()) + " bytes)");
            Response response = StaticHandler::handleError("content_too_large");
            std::string rawResponse = response.stringify();
			_epollControl(clientFd, EPOLL_CTL_MOD, EPOLLOUT, "error response after oversized request");
            return;
        }
		currentReq.append(buff, bytesReceived);
		Log::dev("event", "Received " + utils::str(bytesReceived) + " bytes from client fd " + Log::hl(clientFd));
		if (RequestParser::isRequestComplete(currentReq)) {
			_epollControl(clientFd, EPOLL_CTL_MOD, EPOLLOUT, "request completion"); // throw
			Log::prod("ok", "Request complete on fd " + Log::hl(clientFd) + ". Switching to Send.");
		}
	} else if (bytesReceived == 0) {
		_disconnectClient(clientFd); // finished reading, disconnect client
	} else { // bytes == -1
		Log::dev("event", "Recv would block on fd " + Log::hl(clientFd) + ", waiting for next epoll event");
		// Stay on EPOLLIN
	}
}

void Network::_dispatchAndSendResponse(int clientFd)
{
	try {
		if (_pendingRequests.find(clientFd) == _pendingRequests.end())
				throw std::runtime_error("No request data for client.");
		if (_clientServerMap.find(clientFd) == _clientServerMap.end())
			throw std::runtime_error("Client FD not mapped to any server socket.");

		// 1. Process
		Request request(_pendingRequests[clientFd]);
		Socket* listeningSocket = _clientServerMap[clientFd];
		HostPortPair listenDirective = listeningSocket->getListenDirective();
		Log::prod("event", request.getMethod() + " request received on " + Log::hl(listenDirective) + ".");
		Log::dev("debug", "Request:\n" + utils::str(request));
		Response response = Router::dispatchRequest(_config, request, listenDirective);
		Log::dev("debug", "Response:\n" + utils::str(response));

		// 2. Prepare progressive sending
		std::string rawResp = response.stringify();
		_pendingResponses[clientFd] = rawResp;
		_responseSendPos[clientFd] = 0;
		_shouldCloseAfterResponse[clientFd] = request.isConnectionClose() || response.isConnectionClose();
		ssize_t bytesSent = send(clientFd, rawResp.data(), rawResp.length(), 0);
		if (bytesSent < 0) { // sending error -> store to send later (stay on EPOLLOUT)
			Log::prod("error", "Send failed on fd " + Log::hl(clientFd) + ": " + strerror(errno));
            _disconnectClient(clientFd);
            return;
		}
		Log::dev("event", "Sent " + Log::hl(rawResp.length()) + " bytes to client fd " + Log::hl(clientFd) + ".");

		// 3. Start sending
		_continuePendingSend(clientFd);

	}
	catch (std::exception& e){
		Log::prod("error", "Problem during send/dispatch: " + utils::str(e.what()));
		_disconnectClient(clientFd);
	}
}

// TODO translate in english!
/**
 * Continues sending a pending response for a client.
 * Called when EPOLLOUT indicates the client is ready for more data.
 */
void Network::_continuePendingSend(int clientFd)
{
	if (_pendingResponses.find(clientFd) == _pendingResponses.end()) {
		Log::dev("warning", "No pending response for fd " + utils::str(clientFd));
		return;
	}

	std::string& response = _pendingResponses[clientFd];
	size_t& sendPos = _responseSendPos[clientFd];

	// Envoyer la partie restante de la réponse
	size_t remaining = response.length() - sendPos;
	ssize_t bytesSent = send(clientFd, response.data() + sendPos, remaining, 0);

	if (bytesSent < 0) {
		// Erreur d'envoi - déconnecter le client
		Log::prod("error", "Send failed for fd " + Log::hl(clientFd) + " during continued send");
		_disconnectClient(clientFd);
		return;
	}
	sendPos += bytesSent;

	// Vérifier si l'envoi est complet
	if (sendPos >= response.length()) {
		Log::prod("event", "Response fully sent for fd " + Log::hl(clientFd));

		// Nettoyer les structures d'envoi
		_pendingResponses.erase(clientFd);
		_responseSendPos.erase(clientFd);
		// Gérer la connexion
		if (_shouldCloseAfterResponse[clientFd]) {
			_disconnectClient(clientFd);
		} else {
			_prepareClientForNextRequest(clientFd);
		}
		_shouldCloseAfterResponse.erase(clientFd);
	} else {
		Log::dev("event", "Partial send for fd " + Log::hl(clientFd) + ": " + utils::str(sendPos) + "/" + utils::str(response.length()) + " bytes");
		// Rester en mode EPOLLOUT pour continuer l'envoi
    }
}

// EPOLL

/**
 * Creates the main epoll instance for monitoring all file descriptors.
 * This is the core of our event-driven, non-blocking I/O system.
 */
void Network::_startEpoll()
{
	_epollFd = epoll_create(1);
	if (_epollFd < 0) {
		Log::prod("status", "Epoll status: " + utils::str(strerror(errno)));
		throw std::runtime_error("Can't create epoll.");
	}
}

/**
 * Adds all server listening sockets to epoll monitoring (via their fd).
 * These sockets will detect incoming client connections.
 */
void Network::_addListeningSocketsToEpoll()
{
	for (size_t i = 0; i < _listeningSockets.size(); ++i) {
		int serverFd = _listeningSockets[i]->getFd();
		_epollControl(serverFd, EPOLL_CTL_ADD, EPOLLIN | EPOLLOUT, "server setup"); // throw
	}
}

/**
 * Throws exception if epoll_ctl fails when method action is not EPOLL_CTL_DEL.
 */
void Network::_epollControl(int fd, int operation, uint32_t events, const std::string& context)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;
    if (epoll_ctl(_epollFd, operation, fd, &event) == -1) {
        std::string errorMsg = "epoll_ctl(" + _epollOpToString(operation) + ") failed during " + context + " for fd " + Log::hl(fd) + ": " + utils::str(strerror(errno));
		if (operation == EPOLL_CTL_DEL) {
            Log::dev("warning", errorMsg);
            return; // not critical
        }
        if (_clientServerMap.find(fd) != _clientServerMap.end()) { // fd is a client, not a server
            Log::prod("error", errorMsg);
            return; // not critical
		}
		Log::prod("error", errorMsg);
		throw std::runtime_error("Critical epoll_ctl failure on server socket");
	}
}

/**
 * Waits for I/O activity on monitored file descriptors.
 * Populates "events" with active I/O events.
 * Returns number of file descriptors of ready events.
 */
int Network::_waitAndCollectEvents(struct epoll_event* events)
{
	int readyEventsCount = epoll_wait(_epollFd, events, _MAX_NB_OF_EVENTS, -1);
	if (readyEventsCount == -1) {
		if (errno == EINTR) { // signal received (not an error).
			//!\ errno is not checked after a read or write here -> ok with subject
			return 0; // main loop in startServers will verify sig::keepRunning() and stop webserv
		}
		// else: reel error
		Log::prod("status", "Epoll Wait status: " + utils::str(strerror(errno)));
		throw std::runtime_error("Can't wait events.");
	}
	return (readyEventsCount);
}

std::string	Network::_epollOpToString(int operation)
{
	switch (operation) {
		case EPOLL_CTL_ADD: return "ADD";
		case EPOLL_CTL_MOD: return "MOD";
		case EPOLL_CTL_DEL: return "DEL";
		default: return "UNKNOWN";
	}
}


// CONNECTION


/**
 * Checks if the fd is a listening socket.
 */
bool Network::_isListeningSocket(int fd)
{
    for (size_t i = 0; i < _listeningSockets.size(); ++i) {
        if (fd == _listeningSockets[i]->getFd()) {
            return true;
        }
    }
    return false;
}

/**
 * Accepts a new client connection on a listening socket.
 */
int Network::_acceptNewClient(int listeningFd)
{
    for (size_t i = 0; i < _listeningSockets.size(); ++i) {
        if (listeningFd == _listeningSockets[i]->getFd()) {
            int clientFd =  _listeningSockets[i]->createNewClientSocket();
			if (clientFd > 0)
				_clientServerMap[clientFd] = _listeningSockets[i];
			return clientFd;
        }
    }
    return -1; // Should not happen if _isListeningSocket returned true
}

void Network::_prepareClientForNextRequest(int clientFd)
{
	_pendingRequests.erase(clientFd);
	_pendingResponses.erase(clientFd);
    _responseSendPos.erase(clientFd);
    _shouldCloseAfterResponse.erase(clientFd);
	Log::dev("event", "Connection: keep-alive -> Resetting fd " + Log::hl(clientFd) + " to 'recv'.");
	_epollControl(clientFd, EPOLL_CTL_MOD, EPOLLIN, "keep-alive reset"); // throw
}

void Network::_disconnectClient(int clientFd)
{
	Log::prod("event", "Client " + Log::hl(clientFd)
		+ " disconnected gracefully."); // log before to ensure clientFd is still valid
	_clientServerMap.erase(clientFd);
	_pendingRequests.erase(clientFd);
	_pendingResponses.erase(clientFd);
    _responseSendPos.erase(clientFd);
    _shouldCloseAfterResponse.erase(clientFd);
	_epollControl(clientFd, EPOLL_CTL_DEL, 0, "client disconnect"); // throw
	close(clientFd);
}


// GETTERS

int Network::getEpollFd() const
{
	return _epollFd;
}

std::vector<Socket*> const& Network::getListeningSockets() const
{
	return _listeningSockets;
}

std::map<int, std::string> const& Network::getPendingRequests() const
{
	return _pendingRequests;
}

std::map<int, Socket*> const& Network::getClientServerMap() const
{
	return _clientServerMap;
}

// PRINT

std::ostream& operator<<(std::ostream& os, Network const& rhs)
{
	os << "- Epoll fd: " << rhs.getEpollFd() << "\n";

	os << "- Listening Sockets: " << rhs.getListeningSockets().size() << "\n";
	const std::vector<Socket*>& sockets = rhs.getListeningSockets();
	for (size_t i = 0; i < sockets.size(); ++i)
		os << "  - Socket " << i << " (fd" << sockets[i]->getFd() << ") -> " << sockets[i]->getListenDirective() << "\n";

	os << "- Pending Requests (waiting send): " << rhs.getPendingRequests().size() << "\n";
	os << "- Active Client Connections: " << rhs.getClientServerMap().size() << "\n";

	return os;
}