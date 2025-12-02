#include "network/Network.hpp"

size_t const	Network::_CLIENT_BUFFER_SIZE = 1024 * 1024; // 1MB
size_t const	Network::_MAX_NB_OF_EVENTS = 100;

Network::Network(Config const& config)
: _config(config)
, _epollFd(-1)
, _cgi(this) // CGI handler instance
{
	_initListeningSockets();
}

Network::~Network()
{
	_cgi.fullCleanup();

	_cleanupListeningSockets();
	if (_epollFd != -1)
		close(_epollFd);
	Log::dev("close", "Epoll instance stopped.");

	Log::prod("ok", Const::SERVER_NAME + " closed.");
	std::cout << std::endl;
}

// MAIN LOGIC

/**
 * Main server loop. Waits for I/O activity through epoll and dispatches events between new connections and active clients.
 * Runs in non-blocking mode until the server is stopped.
 *
 * @note We use a single epoll_wait() loop as required by the subject.
 * All I/O operations (read/write on clients and listening sockets) are performed only when epoll signals EPOLLIN or EPOLLOUT.
 * No read() or write() is ever executed outside this event loop.
 *
 * @details `eventFd` can be the file descriptor of:
 * a listening socket (new client connexion),
 * an existing client (data to read/write),
 * or a CGI Pipe (output)
 */
void Network::startServers()
{
	_createEpollInstance(); // create the fd for epoll instance
	_registerListeningSocketsToEpoll(); // add listening sockets to epoll surveillance
	Log::prod("ok", Const::SERVER_NAME + " started.");
	Log::prod("info", "Press Ctrl + C to stop the Web server.");

	struct epoll_event events[_MAX_NB_OF_EVENTS];
	while (sig::keepRunning()) {
		int readyEventsCount = _waitAndCollectEvents(events);

		for (int n = 0; n < readyEventsCount && sig::keepRunning(); n++) {
			int eventFd = events[n].data.fd;
			uint32_t ev = events[n].events;

			if (_isListeningSocket(eventFd)) { // 1. Event on a listening socket -> a new client is trying to connect.
				int newClientFd = _acceptNewClient(eventFd); // create a new client socket
				if (newClientFd > 0) {
					_registerNewClientToEpoll(newClientFd); // add the new client to epoll surveillance
				} else {
					Log::prod("error", "Accept failed on listening socket.");
				}
			} else if (_cgi.isCgiPipe(eventFd)) { // 2a. eventFd corresponds to an existing CGI pipe
				if (ev & (EPOLLERR | EPOLLHUP)) { // pipe error
					_cgi.errorFromPipeFd(eventFd, "internal_error", "CGI pipe error");
				} else if (ev & EPOLLIN) { // ready for reading CGI output from pipes
					_cgi.readAndAccumulateCgiOutput(eventFd);
				}
			} else { // 2c. eventFd corresponds to an existing client socket in epoll
				if (ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
					_disconnectClient(eventFd);
				} else if (ev & EPOLLIN) { // ready for reading raw request from client socket
					_readClientRequest(eventFd);
				} else if (ev & EPOLLOUT) { // ready for writing raw response to client socket
					if (_pendingResponses.find(eventFd) != _pendingResponses.end()) {
						_continuePendingSend(eventFd);
					} else {
						_dispatchAndSendResponse(eventFd);
					}
				}
			}
		}
		if (!_cgi.getContextsByPid().empty())
			_cgi.checkCompletion();
	}
}


/**
 * Reads the configuration and creates one listening socket for each unique port.
 *
 * Creates one socket per port, even if multiple websites share that port.
 * This allows virtual hosts to work properly.
 */
void	Network::_initListeningSockets()
{
	// Get all unique ports only
    std::vector<size_t> usedPorts;
	std::vector<HostPortPair> listenPorts = _config.getAllListenPorts();

	// Only create socket if port is unique
	try {
		for (size_t i = 0; i < listenPorts.size() && sig::keepRunning(); ++i) {
            size_t port = listenPorts[i].getPort();
            bool found = false;
            for (size_t j = 0; j < usedPorts.size(); ++j) {
                if (usedPorts[j] == port) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                usedPorts.push_back(port);
                _listeningSockets.push_back(new Socket(listenPorts[i]));
            }
        }
		//  Bind and listen to each socket created
		for (size_t i = 0; i < _listeningSockets.size(); ++i) {
			_listeningSockets[i]->safeBind();
			_listeningSockets[i]->safeListen();
		}
	} catch (...) {
		_cleanupListeningSockets();
		throw;
	}
}

void	Network::_cleanupListeningSockets()
{
    for (size_t i = 0; i < _listeningSockets.size(); ++i) {
        if (_listeningSockets[i]) {
            delete _listeningSockets[i];
            _listeningSockets[i] = NULL;
        }
    }
    _listeningSockets.clear();
}

/**
 * Configures the client socket as non-blocking and registers it in epoll.
 * Allows receiving and sending data without blocking the server.
 */
void	Network::_registerNewClientToEpoll(int clientFd)
{
	// Configuration of client's socket
	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1
	|| fcntl(clientFd, F_SETFD, FD_CLOEXEC) == -1) {
		Log::prod("error", "fcntl failed for on fd " + utils::str(clientFd) + ": " + strerror(errno));
		close(clientFd);
		return;
	}
	// Add to epoll
	epollControl(clientFd, EPOLL_CTL_ADD, EPOLLIN, "new client setup"); // throw
	Log::dev("event", "New client fd " + Log::hl(clientFd) + " configured and added to epoll");
}

/**
 * Reads data sent by a client. Accumulates the HTTP request until it is complete, then switches the fd to send mode.
 *
 * @note This method is protected by epoll (EPOLLIN) in the epoll main loop (_startServers)
 * and is non-blocking (reading by buffer chunks).
 * This is the only place we read from a client socket.
 * No errno-based decision-making occurs after recv().
 */
void Network::_readClientRequest(int clientFd)
{
	char buff[_CLIENT_BUFFER_SIZE];
	std::string& currentReq = _pendingRequests[clientFd];
	int bytesReceived = recv(clientFd, buff, _CLIENT_BUFFER_SIZE, 0);
	if (bytesReceived > 0) {
		if (currentReq.size() + bytesReceived > Const::ABSOLUTE_MAX_CLIENT_BODY_SIZE) {
            Log::prod("error", "413 Payload Too Large from fd " + Log::hl(clientFd) + " (" + utils::str(currentReq.size()) + " bytes)");
            Response response = StaticHandler::builtinError("content_too_large", "GET");
            std::string rawResponse = response.stringify();
			epollControl(clientFd, EPOLL_CTL_MOD, EPOLLOUT, "error response after oversized request");
            return;
        }
		currentReq.append(buff, bytesReceived);
		Log::dev("event", "Received " + utils::str(bytesReceived) + " bytes from client fd " + Log::hl(clientFd));
		if (RequestParser::isRawRequestComplete(currentReq)) {
			Log::dev("event", "Request fully received from client (fd " + Log::hl(clientFd) + ").");
			_dispatchAndSendResponse(clientFd);
		}
	} else if (bytesReceived == 0) {
		_disconnectClient(clientFd); // finished reading -> disconnect client
	} else { // bytes == -1
		Log::dev("event", "Recv would block on fd " + Log::hl(clientFd) + ", waiting for next epoll event");
		// Stay on EPOLLIN
	}
}

/**
 * Parses the completed HTTP request, generates the response, prepares progressive sending, and sends the first chunk.
 */
void Network::_dispatchAndSendResponse(int clientFd)
{
	try {
		if (_pendingRequests.find(clientFd) == _pendingRequests.end())
			throw std::runtime_error("No request data for client.");
		if (_socketsByClientFd.find(clientFd) == _socketsByClientFd.end())
			throw std::runtime_error("Client FD not mapped to any server socket.");

		// 1. Process
		Request request(_pendingRequests[clientFd]);
		Socket* listeningSocket = _socketsByClientFd[clientFd];
		HostPortPair listenDirective = listeningSocket->getListenDirective();
		Log::prod("ok", request.getMethod() + " request received on " + Log::hl(listenDirective) + " (fd " + Log::hl(clientFd) + ").");
		Log::dev("debug", "Request:\n" + utils::str(request));
		Response response = Router::dispatchRequest(_config, request, listenDirective);

		// CGI -> async response (stays in EPOLLIN to wait for CGI output)
		if (response.needsCgiExecution()) {
			_cgi.launchAsync(clientFd, response);
			return; // => Stay in EPOLLIN
		}
		// Static or redirection -> immediate response (switch to EPOLLOUT to send response)
		prepareResponseSend(clientFd, response);
		epollControl(clientFd, EPOLL_CTL_MOD, EPOLLOUT, "static response ready"); // => Switch to EPOLLOUT
	}
	catch (std::exception& e){
		Log::prod("error", "Problem during send/dispatch: " + utils::str(e.what()));
		_disconnectClient(clientFd);
	}
}

void	Network::prepareResponseSend(int clientFd, Response const& response)
{
    std::string rawResponse = response.stringify();
    _pendingResponses[clientFd] = rawResponse;
    _responseSendPos[clientFd] = 0;
    _shouldCloseAfterResponse[clientFd] = response.isConnectionClose();

    Log::dev("debug", "Response:\n" + utils::str(response));
    Log::dev("event", "Queued " + Log::hl(rawResponse.size()) + " bytes to fd " + Log::hl(clientFd));
}



/**
 * Sends the next part of the pending HTTP response.
 * Continues until everything is sent or the next EPOLLOUT event.
 * Called when EPOLLOUT indicates the client is ready for more data.
 *
 * @note This method is protected by epoll (EPOLLOUT) and is non-blocking (sending by chunks).
 * This is the only place we write to a client socket.
 * No errno-based decision-making occurs after send().
 */
void Network::_continuePendingSend(int clientFd)
{
	if (_pendingResponses.find(clientFd) == _pendingResponses.end()) {
		Log::dev("warning", "No pending response for fd " + utils::str(clientFd));
		return;
	}

	std::string& response = _pendingResponses[clientFd];
	size_t& sendPos = _responseSendPos[clientFd];

	// Send remaining part of the response
	size_t remaining = response.length() - sendPos;
	ssize_t bytesSent = send(clientFd, response.data() + sendPos, remaining, 0);

	if (bytesSent < 0) {
		// Sending error -> Disconnect client
		Log::prod("error", "Send failed for fd " + Log::hl(clientFd) + " during continued send");
		_disconnectClient(clientFd);
		return;
	}
	sendPos += bytesSent;

	// Check if send if complete
	if (sendPos >= response.size()) {
		Log::prod("ok", "Response sent to client (fd " + Log::hl(clientFd) + ").");
		// Cleanup
		_pendingResponses.erase(clientFd);
		_responseSendPos.erase(clientFd);
		// Handle connection
		if (_shouldCloseAfterResponse[clientFd]) {
			_disconnectClient(clientFd);
		} else {
			_prepareClientForNextRequest(clientFd);
		}
		_shouldCloseAfterResponse.erase(clientFd);
	} else {
		Log::dev("event", "Partial send for fd " + Log::hl(clientFd) + ": " + utils::str(sendPos) + "/" + utils::str(response.length()) + " bytes");
		// Stay in EPOLLOUT mode to continue sending
    }
}

// EPOLL

/**
 * Creates the main epoll instance.
 * Central component used to monitor all sockets efficiently (non-blocking).
 */
void Network::_createEpollInstance()
{
	_epollFd = epoll_create(1);
	if (_epollFd < 0) {
		Log::prod("status", "Epoll status: " + utils::str(strerror(errno)));
		throw std::runtime_error("Can't create epoll.");
	}
	Log::dev("setup", "Epoll instance created (fd " + Log::hl(_epollFd) + ").");
}

/**
 * Registers all server listening sockets in epoll.
 * These sockets trigger events when new clients attempt to connect.
 */
void Network::_registerListeningSocketsToEpoll()
{
	for (size_t i = 0; i < _listeningSockets.size(); ++i) {
		int socketFd = _listeningSockets[i]->getFd();
		epollControl(socketFd, EPOLL_CTL_ADD, EPOLLIN | EPOLLOUT, "server setup"); // throw
		Log::dev("setup", "Socket (fd " + Log::hl(socketFd) + ") added to epoll surveillance.");
		Log::prod("ok", Const::SERVER_NAME + " will listen on " + Log::hl(_listeningSockets[i]->getListenDirective()) + " (socket fd " + Log::hl(socketFd) + ").");
	}
}

/**
 * Safe wrapper around epoll_ctl. Logs the operation and handles errors depending on the type of action.
 */
void Network::epollControl(int fd, int operation, uint32_t events, const std::string& context)
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
        if (_socketsByClientFd.find(fd) != _socketsByClientFd.end()) { // fd is a client, not a server
            Log::prod("error", errorMsg);
            return; // not critical
		}
		Log::prod("error", errorMsg);
		throw std::runtime_error("epoll_ctl " + _epollOpToString(operation) + " failure on fd " + Log::hl(fd) + ".");
	}
}

/**
 * Waits until epoll reports activity. Fills the event array with ready descriptors and returns their count.
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

/**
 * Simple helper to convert epoll control operations (int) as a string.
 */
std::string	Network::_epollOpToString(int operation)
{
	switch (operation) {
		case EPOLL_CTL_ADD: return "ADD";
		case EPOLL_CTL_MOD: return "MOD";
		case EPOLL_CTL_DEL: return "DEL";
		default: return "UNKNOWN";
	}
}


// CONNECTIONS

/**
 * Checks whether a given file descriptor belongs to one of the server's listening sockets.
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
 * Calls accept() on the listening socket that triggered the event.
 * Returns a new client fd and stores its associated server socket.
 */
int Network::_acceptNewClient(int listeningFd)
{
    for (size_t i = 0; i < _listeningSockets.size(); ++i) {
        if (listeningFd == _listeningSockets[i]->getFd()) {
            int clientFd =  _listeningSockets[i]->createNewClientSocket();
			if (clientFd > 0)
				_socketsByClientFd[clientFd] = _listeningSockets[i];
			return clientFd;
        }
    }
    return -1; // Should not happen if _isListeningSocket returned true
}

/**
 * Resets the client's internal state so it can process another request on a keep-alive connection.
 */
void Network::_prepareClientForNextRequest(int clientFd)
{
	_pendingRequests.erase(clientFd);
	_pendingResponses.erase(clientFd);
    _responseSendPos.erase(clientFd);
    _shouldCloseAfterResponse.erase(clientFd);
	Log::dev("event", "Connection: keep-alive -> Resetting fd " + Log::hl(clientFd) + " to 'recv'.");
	epollControl(clientFd, EPOLL_CTL_MOD, EPOLLIN, "keep-alive reset"); // throw
}

/**
 * Removes the client from epoll, clears all associated state, and closes the socket.
 * Used for errors or normal closure.
 */
void Network::_disconnectClient(int clientFd)
{
	Log::prod("event", "Client " + Log::hl(clientFd)
		+ " disconnected gracefully."); // log before to ensure clientFd is still valid
	_socketsByClientFd.erase(clientFd);
	_pendingRequests.erase(clientFd);
	_pendingResponses.erase(clientFd);
    _responseSendPos.erase(clientFd);
    _shouldCloseAfterResponse.erase(clientFd);
	epollControl(clientFd, EPOLL_CTL_DEL, 0, "client disconnect"); // throw
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

std::map<int, Socket*> const& Network::getSocketsByClientFd() const
{
	return _socketsByClientFd;
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
	os << "- Active Client Connections: " << rhs.getSocketsByClientFd().size() << "\n";

	return os;
}