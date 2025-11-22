#include "network/Network.hpp"

size_t const	Network::_CLIENT_BUFFER = 1024 * 1024; // 1MB
size_t const	Network::_MAX_NB_OF_EVENTS = 100;

Network::Network(Config const& config)
: _config(config)
, _epollFd(-1)
{
	// Get all unique host:port pairs directly from config
	std::vector<HostPortPair> listenPorts = _config.getAllListenPorts();

	// Create socket for each HostPortPair
	for (size_t i = 0; i < listenPorts.size() && sig::keepRunning(); ++i)
		_connections.push_back(new Socket(listenPorts[i]));

	//  Bind and listen to each socket created
	for (size_t i = 0; i < _connections.size(); ++i) {
		_connections[i]->bind();
		_connections[i]->listen();
	}
}

Network::~Network()
{
	Log::dev("close", "Closing Web server...");

	Log::dev("close", "Freeing Socket memory.");
	for (size_t i = 0; i < _connections.size(); ++i)
		delete _connections[i];

	Log::dev("close", "Closing epoll...");
	if (_epollFd != -1)
		close(_epollFd);

	Log::prod("ok", Const::SERVER_NAME + " closed.");
	std::cout << std::endl;
}


// MAIN LOGIC

void Network::startServers()
{
	Log::prod("ok", Const::SERVER_NAME + " started.");
	Log::prod("info", "Press Ctrl + C to stop the Web server.");

	_epollCreate(); // set _epollFd
	_epollAddServers(); // add "server" blocks' sockets to the surveillance

	struct epoll_event events[_MAX_NB_OF_EVENTS];
	while (sig::keepRunning()) {
		int readyEventsCount = _epollWait(events);
		for (int n = 0; n < readyEventsCount && sig::keepRunning(); n++) {
			int eventFd = events[n].data.fd;
			int clientFd = _acceptConnection(eventFd);
			if (clientFd > 0) { // 1. New client connection
				_addClientToEpoll(clientFd);
			} else if (clientFd == 0) { // 2. CGI or existing client
				if (events[n].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) // error
					_handleClientDisconnect(eventFd);
				//else if (_isCgiEvent(eventFd)) // CGI // TODO
            	//	_handleCgiOutput(eventFd);
        		else if (events[n].events & EPOLLIN) // can recieve from client
					_readClientRequest(eventFd);
				else if (events[n].events & EPOLLOUT) // can send to client
					_dispatchAndSendResponse(eventFd); // also prepare CGI pipes
			} else { // 3. Connexion error
				Log::prod("error", "Accept failed on server socket");
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
	char buff[_CLIENT_BUFFER];
	std::string& currentReq = _pendingRequests[clientFd];
	int bytes;

	bytes = _safeRecv(clientFd, buff, _CLIENT_BUFFER);
	if (bytes > 0) {
		currentReq.append(buff, bytes); // data received, continue reading
		Log::dev("event", "Received " + utils::str(bytes) + " bytes from fd " + Log::hl(clientFd));
		if (RequestParser::isRequestComplete(currentReq)) {
			_epollControl(clientFd, EPOLL_CTL_MOD, EPOLLOUT, "request completion"); // throw
			Log::prod("ok", "Request complete on fd " + Log::hl(clientFd) + ". Switching to Send.");
		}
	} else if (bytes == 0) {
		_handleClientDisconnect(clientFd); // finished reading, disconnect client
	} else { // bytes == -1
		Log::dev("event", "No data available yet for fd " + Log::hl(clientFd) + ", waiting...");
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
		Socket* serverSocket = _clientServerMap.at(clientFd);
		HostPortPair listenPair = serverSocket->getHostPortPair();
		Log::prod("event", request.getMethod() + " request received on " + Log::hl(listenPair) + ".");
		Log::dev("debug", "Request:\n" + utils::str(request));
		Response response = Router::dispatchRequest(_config, request, listenPair);
		Log::dev("debug", "Response:\n" + utils::str(response));

		// 2. Send
		std::string msg = response.stringify();
		ssize_t ret = _safeSend(clientFd, msg.data(), msg.length());
		if (ret == -1)
			throw std::runtime_error(std::string("Send failed: ") + strerror(errno));

		// 3. Manage the connection (Close/keep logic moved here)
		bool shouldClose = request.isConnectionClose() || response.isConnectionClose();
		_manageConnection(clientFd, shouldClose);

		Log::prod("event", "response \"" + response.getStatus().toStr() + "\" sent on " + Log::hl(listenPair) + ".");
	}
	catch (std::exception& e){
		Log::prod("error", "Problem during send/dispatch: " + utils::str(e.what()));
		_manageConnection(clientFd, true); // Force to close
	}
}

// EPOLL

/**
 * Creates the main epoll instance for monitoring all file descriptors.
 * This is the core of our event-driven, non-blocking I/O system.
 */
void Network::_epollCreate()
{
	_epollFd = epoll_create(1);
	if (_epollFd < 0) {
		Log::prod("status", "Epoll status: " + utils::str(strerror(errno)));
		throw std::runtime_error("Can't create epoll.");
	}
}

/**
 * Adds all server listening sockets to epoll monitoring.
 * These sockets will detect incoming client connections.
 */
void Network::_epollAddServers()
{
	for (size_t i = 0; i < _connections.size(); ++i) {
		int serverFd = _connections[i]->getFd();
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
int Network::_epollWait(struct epoll_event* events)
{
	int readyEventsCount = epoll_wait(_epollFd, events, _MAX_NB_OF_EVENTS, -1);
	if (readyEventsCount == -1) {
		if (errno == EINTR) // signal received (not an error)
			return 0; // main loop in startServers will verify sig::keepRunning() and stop webserv
		// else: reel error
		Log::prod("status", "Epoll Wait status: " + utils::str(strerror(errno)));
		throw std::runtime_error("Can't wait events.");
	}
	return (readyEventsCount);
}

ssize_t	Network::_safeRecv(int fd, void* buf, size_t size)
{
	struct pollfd p;
	p.fd = fd;
	p.events = POLLIN;

	int r = poll(&p, 1, 0);  // Timeout = 0 → non-blocking
	if (r <= 0 || !(p.revents & POLLIN))
		return 0; // No data available, or not ready

	return recv(fd, buf, size, 0);
}

ssize_t	Network::_safeSend(int fd, const void* buf, size_t size)
{
	struct pollfd p;
	p.fd = fd;
	p.events = POLLOUT;

	int r = poll(&p, 1, 0); // Timeout = 0 → non-blocking
	if (r <= 0 || !(p.revents & POLLOUT))
		return 0; // Not ready for writing

	return send(fd, buf, size, 0);
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
 * Returns the new fd if the event is accepted, 0 if not accepted, -1 if accept() failed.
 */
int Network::_acceptConnection(int serverFd)
{
	// Iterate through listening sockets (servers)
	for (size_t i = 0; i < _connections.size(); ++i) {
		// IF and ONLY IF the event fd equals one of my server sockets
		if (serverFd == _connections[i]->getFd()) {
			Log::dev("event", "Server side event on fd " + Log::hl(serverFd) + ".");
			int clientFd = _connections[i]->accept();
			if (clientFd > 0)
				_clientServerMap[clientFd] = _connections[i];
			return (clientFd);
		}
	}
	// If not a server event, don't print anything and return 0
	return (0);
}

void Network::_manageConnection(int clientFd, bool shouldClose)
{
	_pendingRequests.erase(clientFd);
	if (shouldClose) {
		_handleClientDisconnect(clientFd);
	} else {
		Log::dev("event", "Connection: keep-alive -> Resetting fd " + Log::hl(clientFd) + " to 'recv'.");
		_epollControl(clientFd, EPOLL_CTL_MOD, EPOLLIN, "keep-alive reset"); // throw
	}
}

void Network::_handleClientDisconnect(int clientFd)
{
	Log::prod("event", "Client " + Log::hl(clientFd)
		+ " disconnected gracefully."); // log before to ensure clientFd is still valid
	_clientServerMap.erase(clientFd);
	_pendingRequests.erase(clientFd);
	_epollControl(clientFd, EPOLL_CTL_DEL, 0, "client disconnect"); // throw
	close(clientFd);
}


// GETTERS

int Network::getEpollFd() const
{
	return _epollFd;
}

std::vector<Socket*> const& Network::getConnections() const
{
	return _connections;
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

	os << "- Listening Sockets: " << rhs.getConnections().size() << "\n";
	const std::vector<Socket*>& sockets = rhs.getConnections();
	for (size_t i = 0; i < sockets.size(); ++i)
		os << "  - Socket " << i << " (fd" << sockets[i]->getFd() << ") -> " << sockets[i]->getHostPortPair() << "\n";

	os << "- Pending Requests (waiting send): " << rhs.getPendingRequests().size() << "\n";
	os << "- Active Client Connections: " << rhs.getClientServerMap().size() << "\n";

	return os;
}