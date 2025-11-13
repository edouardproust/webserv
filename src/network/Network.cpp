#include "network/Network.hpp"

Network::Network(Config const& config) : _config(config)
{
	// Get all unique host:port pairs directly from config
	std::vector<HostPortPair> listen_ports = _config.getAllListenPorts();

	// Create socket for each HostPortPair
	for (size_t i = 0; i < listen_ports.size() && sig::keepRunning(); ++i)
		_connections.push_back(new Socket(listen_ports[i]));

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

	Log::prod("ok", SERVER_NAME + " closed.");
	std::cout << std::endl;
}

void Network::startServers()
{
	int number_of_events;
	int new_conn;
	struct epoll_event events[FT_MAX_EVENT_SIZE];
	struct epoll_event events_setup;

	Log::prod("ok", SERVER_NAME + " started.");
	Log::prod("info", "Press Ctrl + C to stop the Web server.");

	epoll();
	epollAddServers();

	while (sig::keepRunning())
	{
		number_of_events = epoll_wait(events);
		for (int n = 0; n < number_of_events && sig::keepRunning(); n++)
		{
			if ((new_conn = isServerSideEvent(events[n].data.fd)) != 0)
			{
				// Forma correta:
				::fcntl(new_conn, F_SETFL, O_NONBLOCK);  // Define as non-blocking
				::fcntl(new_conn, F_SETFD, FD_CLOEXEC); // Define para fechar no exec()
				//::fcntl(new_conn, F_SETFL, O_NONBLOCK, FD_CLOEXEC);
				events_setup.data.fd = new_conn;
				events_setup.events = EPOLLIN;
				if (::epoll_ctl(_epoll, EPOLL_CTL_ADD, new_conn, &events_setup) == -1)
				{
					Log::prod("status", "Epoll Ctl status: " + utils::str(strerror(errno)));
					throw EpollCtlException();
				}
			}
			else if (events[n].events & EPOLLIN)
				recv(events[n].data.fd, events_setup);
			else if (events[n].events & EPOLLOUT)
				send(events[n].data.fd, events_setup);
		}
	}
}

void Network::epoll()
{
	_epoll = ::epoll_create(1);
	if (_epoll < 0)
	{
		Log::prod("status", "Epoll status: " + utils::str(strerror(errno)));
		throw EpollException();
	}
}

void Network::epollAddServers()
{
	struct epoll_event events_setup;

	events_setup.events = EPOLLIN | EPOLLOUT;
	for (size_t i = 0; i < _connections.size(); ++i) {
		events_setup.data.fd = _connections[i]->getSock();
		if (::epoll_ctl(_epoll, EPOLL_CTL_ADD, _connections[i]->getSock(), &events_setup) < 0)
		{
			Log::prod("status", "Epoll Ctl status: " + utils::str(strerror(errno)));
			throw EpollCtlException();
		}
	}
}

int Network::epoll_wait(struct epoll_event *events)
{
	int nfds;

	nfds = ::epoll_wait(_epoll, events, FT_MAX_EVENT_SIZE, -1);
	if (nfds == -1 && sig::keepRunning())
	{
		Log::prod("status", "Epoll Wait status: " + utils::str(strerror(errno)));
		throw EpollWaitException();
	}

	return (nfds);
}

int Network::isServerSideEvent(int epoll_fd)
{
	// Iterate through listening sockets (servers)
	for (size_t i = 0; i < _connections.size(); ++i)
	{
		// IF and ONLY IF the event fd equals one of my server sockets
		if (epoll_fd == _connections[i]->getSock())
		{
			Log::dev("event", "Server side event on fd " + Log::hl(epoll_fd) + ".");
			int new_conn = _connections[i]->accept();
			if (new_conn > 0)
				_client_server_map[new_conn] = _connections[i];
			return (new_conn);
		}
	}
	// If not a server event, don't print anything and return 0
	return (0);
}

void Network::_handleClientDisconnect(int client_fd, struct epoll_event &events_setup)
{
	Log::prod("event", "Client " + utils::str(client_fd) + " disconnected.");
	_client_server_map.erase(client_fd);
	_request_list.erase(client_fd);
	epoll_ctl(_epoll, EPOLL_CTL_DEL, client_fd, &events_setup);
	close(client_fd);
}

void Network::_handleRecvError(int client_fd, struct epoll_event &events_setup)
{
	Log::prod("error", "recv() on client " + utils::str(client_fd) + ": " + utils::str(strerror(errno)));

	_client_server_map.erase(client_fd);
	_request_list.erase(client_fd);
	epoll_ctl(_epoll, EPOLL_CTL_DEL, client_fd, &events_setup);
	close(client_fd);
}

void Network::recv(int client_fd, struct epoll_event &events_setup)
{
	Log::dev("event", "recv() on fd " + Log::hl(client_fd) + ".");

	char client_buffer[FT_DEFAULT_CLIENT_BUFFER_SIZE];
    std::string& total_request = _request_list[client_fd];
	int bytes;

	while (sig::keepRunning())
	{
		bytes = ::recv(client_fd, client_buffer, FT_DEFAULT_CLIENT_BUFFER_SIZE, 0);

		if (bytes > 0)
		{
			Log::dev("event", "Received " + utils::str(bytes) + " bytes.");
			total_request.append(client_buffer, bytes);
		}
		else if (bytes == 0)
		{
			_handleClientDisconnect(client_fd, events_setup);
			return;
		}
		else // bytes == -1
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break; // No more data to read right now
			else
			{
				_handleRecvError(client_fd, events_setup);
				return;
			}
		}
	}
	if (!total_request.empty() && total_request.find("\r\n\r\n") != std::string::npos)
	{
		events_setup.data.fd = client_fd;
		events_setup.events = EPOLLOUT;
		epoll_ctl(_epoll, EPOLL_CTL_MOD, client_fd, &events_setup);
	}
}

void Network::send(int client_fd, struct epoll_event &events_setup)
{
	Log::dev("event", "send() on fd " + Log::hl(client_fd) + ".");

	Response response;
	try {
		Request request(_request_list[client_fd]);
		if (_client_server_map.find(client_fd) == _client_server_map.end())
			throw std::runtime_error("Network logic error: client_fd not in map.");
		Socket* serverSocket = _client_server_map.at(client_fd);
		HostPortPair listenPair = serverSocket->getHostPortPair();

		Log::prod("event", request.getMethod() + " request received on " + Log::hl(listenPair) + ".");
		Log::dev("debug", "Request:\n" + utils::str(request));

		// Call router
		response = Router::dispatchRequest(_config, request, listenPair);
		Log::dev("debug", "Response:\n" + utils::str(response));

		std::string msg = response.stringify();
		int ret = ::send(client_fd, msg.data(), msg.length(), 0);
		if (ret == -1)
			throw std::runtime_error("Send failed");
		_request_list.erase(client_fd);
		_client_server_map.erase(client_fd);
		events_setup.data.fd = client_fd;
		epoll_ctl(_epoll, EPOLL_CTL_DEL, client_fd, &events_setup);
		close(client_fd);

		Log::prod("event", "response \"" + response.getStatus().toStr() + "\" sent on " + Log::hl(listenPair) + ".");
	}
	catch (std::exception& e) {
		Log::prod("error", "Problem during send/dispatch: " + utils::str(e.what()));
        _request_list.erase(client_fd);
		_client_server_map.erase(client_fd);
		epoll_ctl(_epoll, EPOLL_CTL_DEL, client_fd, &events_setup);
		close(client_fd);
	}
}

int Network::getEpollFd() const {
	return _epoll;
}

const std::vector<Socket*>& Network::getConnections() const {
	return _connections;
}

const std::map<int, std::string>& Network::getRequestList() const {
	return _request_list;
}

const std::map<int, Socket*>& Network::getClientServerMap() const {
	return _client_server_map;
}

std::ostream& operator<<(std::ostream& os, const Network& rhs)
{
	os << "Network:\n";
	os << "- Epoll fd: " << rhs.getEpollFd() << "\n";

	os << "- Listening Sockets: " << rhs.getConnections().size() << "\n";
	const std::vector<Socket*>& sockets = rhs.getConnections();
	for (size_t i = 0; i < sockets.size(); ++i)
		os << "  - Socket " << i << " (fd" << sockets[i]->getSock() << ") -> " << sockets[i]->getHostPortPair() << "\n";

	os << "- Pending Requests (waiting send): " << rhs.getRequestList().size() << "\n";
	os << "- Active Client Connections: " << rhs.getClientServerMap().size() << "\n";

	return os;
}

const char *Network::EpollException::what() const throw()
{
	return ("Can't create epoll.");
}

const char *Network::EpollCtlException::what() const throw()
{
	return ("Can't manage file descriptor.");
}

const char *Network::EpollWaitException::what() const throw()
{
	return ("Can't wait events.");
}