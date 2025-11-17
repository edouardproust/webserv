#include "network/Network.hpp"

size_t const	Network::_CLIENT_BUFFER = 1024;
size_t const	Network::_EVENT_MAX_SIZE = 100;

Network::Network(Config const& config)
: _config(config)
, _epoll(-1)
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
	close(_epoll);

	Log::prod("ok", Const::SERVER_NAME + " closed.");
	std::cout << std::endl;
}

void Network::startServers()
{
	int numberOfEvents;
	int newConn;
	struct epoll_event events[_EVENT_MAX_SIZE];
	struct epoll_event eventsSetup;

	Log::prod("ok", Const::SERVER_NAME + " started.");
	Log::prod("info", "Press Ctrl + C to stop the Web server.");

	_epollCreate();
	_epollAddServers();

	while (sig::keepRunning())
	{
		numberOfEvents = _epoll_wait(events);
		for (int n = 0; n < numberOfEvents && sig::keepRunning(); n++)
		{
			if ((newConn = _isServerSideEvent(events[n].data.fd)) != 0)
			{
				// Correct form:
				::fcntl(newConn, F_SETFL, O_NONBLOCK);  // Define as non-blocking
				::fcntl(newConn, F_SETFD, FD_CLOEXEC); // Marks to be closed on exec()
				//::fcntl(newConn, F_SETFL, O_NONBLOCK, FD_CLOEXEC);
				eventsSetup.data.fd = newConn;
				eventsSetup.events = EPOLLIN;
				if (::epoll_ctl(_epoll, EPOLL_CTL_ADD, newConn, &eventsSetup) == -1)
				{
					Log::prod("status", "Epoll Ctl status: " + utils::str(strerror(errno)));
					throw EpollCtlException();
				}
			}
			else if (events[n].events & EPOLLIN)
				_recv(events[n].data.fd, eventsSetup);
			else if (events[n].events & EPOLLOUT)
				_send(events[n].data.fd, eventsSetup);
		}
	}
}

void Network::_epollCreate()
{
	_epoll = epoll_create(1);
	if (_epoll < 0)
	{
		Log::prod("status", "Epoll status: " + utils::str(strerror(errno)));
		throw EpollException();
	}
}

void Network::_epollAddServers()
{
	struct epoll_event eventsSetup;

	eventsSetup.events = EPOLLIN | EPOLLOUT;
	for (size_t i = 0; i < _connections.size(); ++i) {
		eventsSetup.data.fd = _connections[i]->getSock();
		if (epoll_ctl(_epoll, EPOLL_CTL_ADD, _connections[i]->getSock(), &eventsSetup) < 0)
		{
			Log::prod("status", "Epoll Ctl status: " + utils::str(strerror(errno)));
			throw EpollCtlException();
		}
	}
}

int Network::_epoll_wait(struct epoll_event* events)
{
	int nfds;

	nfds = epoll_wait(_epoll, events, _EVENT_MAX_SIZE, -1);
	if (nfds == -1 && sig::keepRunning())
	{
		Log::prod("status", "Epoll Wait status: " + utils::str(strerror(errno)));
		throw EpollWaitException();
	}

	return (nfds);
}

int Network::_isServerSideEvent(int epollFd)
{
    // Iterate through listening sockets (servers)
    for (size_t i = 0; i < _connections.size(); ++i)
    {
        // IF and ONLY IF the event fd equals one of my server sockets
        if (epollFd == _connections[i]->getSock())
        {
			Log::dev("event", "Server side event on fd " + Log::hl(epollFd) + ".");
            int newConn = _connections[i]->accept();
            if (newConn > 0)
                _clientServerMap[newConn] = _connections[i];
            return (newConn);
        }
    }
    // If not a server event, don't print anything and return 0
    return (0);
}

void Network::_handleClientDisconnect(int clientFd, struct epoll_event& eventsSetup)
{
	Log::prod("event", "Client " + Log::hl(clientFd) + " disconnected.");
	_clientServerMap.erase(clientFd);
	_requestList.erase(clientFd);
	epoll_ctl(_epoll, EPOLL_CTL_DEL, clientFd, &eventsSetup);
	close(clientFd);
}

void Network::_handleRecvError(int clientFd, struct epoll_event &eventsSetup)
{
	Log::prod("error", "recv() on client " + Log::hl(clientFd) + ": " + utils::str(strerror(errno)));

	_clientServerMap.erase(clientFd);
	_requestList.erase(clientFd);
	epoll_ctl(_epoll, EPOLL_CTL_DEL, clientFd, &eventsSetup);
	close(clientFd);
}

void Network::_recv(int clientFd, struct epoll_event& eventsSetup)
{
	Log::dev("event", "recv() on fd " + Log::hl(clientFd) + ".");

    char client_buffer[_CLIENT_BUFFER];
    std::string& totalRequest = _requestList[clientFd];
    int bytes;

    while (sig::keepRunning())
    {
        bytes = recv(clientFd, client_buffer, _CLIENT_BUFFER, 0);

        if (bytes > 0)
        {
			//Log::dev("event", "Received " + utils::str(bytes) + " bytes.");
            totalRequest.append(client_buffer, bytes);
        }
        else if (bytes == 0)
        {
            _handleClientDisconnect(clientFd, eventsSetup);
            return;
        }
        else // bytes == -1
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break; // all bytes read from this event
            }
            else
            {
                _handleRecvError(clientFd, eventsSetup);
                return;
            }
        }
    }
    if (_isRequestComplete(totalRequest))
    {
		Log::prod("ok", "Request complete on fd " + Log::hl(clientFd) + ". Switching to Send.");
        eventsSetup.data.fd = clientFd;
        eventsSetup.events = EPOLLOUT;
        epoll_ctl(_epoll, EPOLL_CTL_MOD, clientFd, &eventsSetup);
    }
    else
    {
		Log::dev("event", "Request incomplete on fd " + Log::hl(clientFd) + ". Waiting for more data.");
    }
}


bool Network::_isRequestComplete(std::string const& totalRequest)
{
    size_t headerEnd = totalRequest.find("\r\n\r\n");

    if (headerEnd == std::string::npos)
    {
        return false; // incomplete headers
    }

    // If headers completed
    std::string headersStr = totalRequest.substr(0, headerEnd);

    // 2a. Check for Chunked Encoding
    if (headersStr.find("Transfer-Encoding: chunked") != std::string::npos ||
        headersStr.find("transfer-encoding: chunked") != std::string::npos)
    {
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

    if (clPos != std::string::npos)
    {
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

void Network::_send(int clientFd, struct epoll_event& eventsSetup)
{
	Log::dev("event", "send() on fd " + Log::hl(clientFd) + ".");

    bool shouldClose = true; // Default is to close in case of error

    try
    {
        if (_requestList.find(clientFd) == _requestList.end()) {
             throw std::runtime_error("No request data for client.");
        }
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

        // 2. Decide (Keep-Alive logic moved here)
        shouldClose = _shouldCloseConnection(request, response);

        // 3. Send
        std::string msg = response.stringify();
        ssize_t ret = send(clientFd, msg.data(), msg.length(), 0);
        if (ret == -1)
            throw std::runtime_error(std::string("Send failed: ") + strerror(errno));

        // 4. Manage the connection (Close/keep logic moved here)
        _manageConnection(clientFd, shouldClose, eventsSetup);

		Log::prod("event", "response \"" + response.getStatus().toStr() + "\" sent on " + Log::hl(listenPair) + ".");
    }
    catch (std::exception& e)
    {
        // The 'catch' block now only needs to close the connection
		Log::prod("error", "Problem during send/dispatch: " + utils::str(e.what()));
        _manageConnection(clientFd, true, eventsSetup); // Força o fecho
    }
}


bool Network::_shouldCloseConnection(Request const& request, Response const& response)
{
    std::string connectionHeader;

    std::map<std::string, std::string> const& reqHeaders = request.getHeaders();
    std::map<std::string, std::string>::const_iterator it = reqHeaders.find("Connection");

    if (it == reqHeaders.end()) {
        it = reqHeaders.find("connection"); // Try lowercase
    }

    if (it != reqHeaders.end()) {
        connectionHeader = it->second; // Header found
        if (connectionHeader == "close") {
            return true; // Client requested to close
        }
    }

    // Rule 2: Did the server decide to close?
    std::map<std::string, std::string> const& respHeaders = response.getHeaders();
    std::map<std::string, std::string>::const_iterator resp_it = respHeaders.find("Connection");

    if (resp_it == respHeaders.end()) {
        resp_it = respHeaders.find("connection");
    }

    if (resp_it != respHeaders.end() && resp_it->second == "close") {
        return true;
    }

    return false; // Default: keep-alive
}

void Network::_manageConnection(int clientFd, bool shouldClose, struct epoll_event& eventsSetup)
{
    _requestList.erase(clientFd);

    if (shouldClose)
    {
		Log::dev("close", "Connection: close -> Closing fd " + Log::hl(clientFd) + ".");
        _clientServerMap.erase(clientFd);
        epoll_ctl(_epoll, EPOLL_CTL_DEL, clientFd, &eventsSetup);
        close(clientFd);
    }
    else
    {
        Log::dev("event", "Connection: keep-alive -> Resetting fd " + Log::hl(clientFd) + " to 'recv'.");
        eventsSetup.data.fd = clientFd;
        eventsSetup.events = EPOLLIN;
        epoll_ctl(_epoll, EPOLL_CTL_MOD, clientFd, &eventsSetup);
    }
}

int Network::getEpollFd() const
{
	return _epoll;
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

std::ostream& operator<<(std::ostream& os, Network const& rhs)
{
	os << "- Epoll fd: " << rhs.getEpollFd() << "\n";

	os << "- Listening Sockets: " << rhs.getConnections().size() << "\n";
	const std::vector<Socket*>& sockets = rhs.getConnections();
	for (size_t i = 0; i < sockets.size(); ++i)
		os << "  - Socket " << i << " (fd" << sockets[i]->getSock() << ") -> " << sockets[i]->getHostPortPair() << "\n";

	os << "- Pending Requests (waiting send): " << rhs.getRequestList().size() << "\n";
	os << "- Active Client Connections: " << rhs.getClientServerMap().size() << "\n";

	return os;
}

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