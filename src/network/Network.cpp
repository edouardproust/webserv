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
		try {
			_connections[i]->bind();
			_connections[i]->listen();
		} catch (Socket::BindException& e) {
			// Cleanup allocated sockets before throwing
			for (size_t j = 0; j < _connections.size(); ++j)
				delete _connections[j];
			_connections.clear();
			throw;
		}
	}
}

Network::~Network()
{
	Log::dev("close", "Closing Web server...");

	Log::dev("close", "Freeing Socket memory.");
	for (size_t i = 0; i < _connections.size(); ++i)
		delete _connections[i];

	Log::dev("close", "Closing epoll...");
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
					_recv(eventFd);
				else if (events[n].events & EPOLLOUT) // can send to client
					_send(eventFd); // also prepare CGI pipes
			} else { // 3. Connexion error
				Log::prod("error", "Accept failed on server socket");
			}
		}
	}
}

void	Network::_addClientToEpoll(int clientFd)
{
	// Configuration of client's socket
	fcntl(clientFd, F_SETFL, O_NONBLOCK); // TODO check if -1
	fcntl(clientFd, F_SETFD, FD_CLOEXEC); // TODO check if -1
	// Add to epoll
	_epollControl(clientFd, EPOLL_CTL_ADD, EPOLLIN, "new client setup");
	Log::dev("event", "New client fd " + Log::hl(clientFd) + " configured and added to epoll");
}

void Network::_recv(int clientFd)
{
	Log::dev("event", "recv() on fd " + Log::hl(clientFd) + ".");

	char buff[_CLIENT_BUFFER];
	std::string& totalRequest = _requestList[clientFd];
	int bytes;

	while (sig::keepRunning()) {
		bytes = recv(clientFd, buff, _CLIENT_BUFFER, 0);
		if (bytes > 0) {
			totalRequest.append(buff, bytes); // data received, continue reading
		} else if (bytes == 0) {
			return _handleClientDisconnect(clientFd); // finished reading, disconnect client
		} else { // bytes == -1
			if (errno == EAGAIN || errno == EWOULDBLOCK) // (EWOULDBLOCK is a historical alias of EAGAIN)
				break; // no more data available for now (non-blocking)
			return _handleClientDisconnect(clientFd); // network error
		}
	}
	if (_isRequestComplete(totalRequest)) {
		_epollControl(clientFd, EPOLL_CTL_MOD, EPOLLOUT, "request completion");
		Log::prod("ok", "Request complete on fd " + Log::hl(clientFd) + ". Switching to Send.");
	} else {
		Log::dev("event", "Request incomplete on fd " + Log::hl(clientFd) + ". Waiting for more data.");
	}
}

void Network::_send(int clientFd)
{
	Log::dev("event", "send() on fd " + Log::hl(clientFd) + ".");

	try {
		if (_requestList.find(clientFd) == _requestList.end())
				throw std::runtime_error("No request data for client.");
		if (_clientServerMap.find(clientFd) == _clientServerMap.end())
			throw std::runtime_error("Client FD not mapped to any server socket.");

		// 1. Process
		Request request(_requestList[clientFd]);
		Socket* serverSocket = _clientServerMap.at(clientFd);
		HostPortPair listenPair = serverSocket->getHostPortPair();
		Log::prod("event", request.getMethod() + " request received on " + Log::hl(listenPair) + ".");
		Log::dev("debug", "Request:\n" + utils::str(request));
		Response response = Router::dispatchRequest(_config, request, listenPair);
		Log::dev("debug", "Response:\n" + utils::str(response));

		// 2. Send
		std::string msg = response.stringify();
		ssize_t ret = send(clientFd, msg.data(), msg.length(), 0);
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

bool Network::_isRequestComplete(std::string const& totalRequest)
{
	size_t headerEnd = totalRequest.find("\r\n\r\n");

	if (headerEnd == std::string::npos)
		return false; // incomplete headers

	// If headers completed
	std::string headersStr = totalRequest.substr(0, headerEnd);

	// 2a. Check for Chunked Encoding
	if (headersStr.find("Transfer-Encoding: chunked") != std::string::npos
			|| headersStr.find("transfer-encoding: chunked") != std::string::npos) {
		if (totalRequest.rfind("0\r\n\r\n") != std::string::npos)
			return true; // Complete chunked body
		// else: Incomplete body
		return false;
	}

	// 2b. Check for Content-Length
	size_t clPos = std::string::npos;
	size_t clLower = headersStr.find("content-length: ");
	size_t clUpper = headersStr.find("Content-Length: ");
	if (clLower != std::string::npos) clPos = clLower + 16;
	else if (clUpper != std::string::npos) clPos = clUpper + 16;

	if (clPos != std::string::npos) {
		// Content-Length found
		std::stringstream ss(headersStr.substr(clPos));
		size_t contentLength = 0;
		ss >> contentLength;
		size_t body_length = totalRequest.length() - (headerEnd + 4);
		if (body_length >= contentLength) {
			return true; // Complete body received!
		}
		// else: incomplete body
		return false;
	}
	return true;
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
		throw EpollException();
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
		_epollControl(serverFd, EPOLL_CTL_ADD, EPOLLIN | EPOLLOUT, "server setup");
	}
}

void Network::_epollControl(int fd, int operation, uint32_t events, const std::string& context)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;
    if (epoll_ctl(_epollFd, operation, fd, &event) == -1) {
        Log::prod("error", "epoll_ctl(" + _epollOpToString(operation) + ") failed during "
			+ context + " for fd " + Log::hl(fd) + ": " + utils::str(strerror(errno)));
		if (operation != EPOLL_CTL_DEL) // DEL is not critical
        	throw EpollCtlException();
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
		throw EpollWaitException();
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
	_requestList.erase(clientFd);
	if (shouldClose) {
		_handleClientDisconnect(clientFd);
	} else {
		Log::dev("event", "Connection: keep-alive -> Resetting fd " + Log::hl(clientFd) + " to 'recv'.");
		_epollControl(clientFd, EPOLL_CTL_MOD, EPOLLIN, "keep-alive reset");
	}
}

void Network::_handleClientDisconnect(int clientFd)
{
	Log::prod("event", "Client " + Log::hl(clientFd)
		+ " disconnected."); // log before to ensure clientFd is still valid
	_clientServerMap.erase(clientFd);
	_requestList.erase(clientFd);
	_epollControl(clientFd, EPOLL_CTL_DEL, 0, "client disconnect");
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

std::map<int, std::string> const& Network::getRequestList() const
{
	return _requestList;
}

std::map<int, Socket*> const& Network::getClientServerMap() const
{
	return _clientServerMap;
}


// EXCEPTIONS

char const*	Network::EpollException::what() const throw()
{
	return ("Can't create epoll.");
}

char const*	Network::EpollCtlException::what() const throw()
{
	return ("Can't manage file descriptor.");
}

char const*	Network::EpollWaitException::what() const throw()
{
	return ("Can't wait events.");
}


// PRINT

std::ostream& operator<<(std::ostream& os, Network const& rhs)
{
	os << "- Epoll fd: " << rhs.getEpollFd() << "\n";

	os << "- Listening Sockets: " << rhs.getConnections().size() << "\n";
	const std::vector<Socket*>& sockets = rhs.getConnections();
	for (size_t i = 0; i < sockets.size(); ++i)
		os << "  - Socket " << i << " (fd" << sockets[i]->getFd() << ") -> " << sockets[i]->getHostPortPair() << "\n";

	os << "- Pending Requests (waiting send): " << rhs.getRequestList().size() << "\n";
	os << "- Active Client Connections: " << rhs.getClientServerMap().size() << "\n";

	return os;
}