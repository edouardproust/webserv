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
	if (DEVMODE) std::cout << FT_CLOSE << "Closing Web server..." << std::endl;

	if(DEVMODE) std::cout << FT_CLOSE << "Freeing Socket memory." << std::endl;
	for (size_t i = 0; i < _connections.size(); ++i)
		delete _connections[i];

	if (DEVMODE) std::cout << FT_CLOSE << "Closing epoll..." << std::endl;
	close(_epoll);

	std::cout << FT_OK << SERVER_NAME << " closed." << std::endl;
	std::cout << std::endl;
}

void Network::startServers()
{
	int number_of_events;
	int new_conn;
	struct epoll_event events[FT_MAX_EVENT_SIZE];
	struct epoll_event events_setup;

	std::cout << FT_OK << SERVER_NAME << " started." << std::endl;
	std::cout << FT_INFO << "Press Ctrl + C to stop the Web server." << std::endl;

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
				::fcntl(new_conn, F_SETFL, O_NONBLOCK);  // Define como non-blocking
				::fcntl(new_conn, F_SETFD, FD_CLOEXEC); // Define para fechar no exec()
				//::fcntl(new_conn, F_SETFL, O_NONBLOCK, FD_CLOEXEC);
				events_setup.data.fd = new_conn;
				events_setup.events = EPOLLIN;
				if (::epoll_ctl(_epoll, EPOLL_CTL_ADD, new_conn, &events_setup) == -1)
				{
					std::cout << FT_STATUS << "Epoll Ctl status: " << strerror(errno) << std::endl;
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
		std::cout << FT_STATUS << "Epoll status: " << strerror(errno) << std::endl;
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
			std::cout << FT_STATUS << "Epoll Ctl status: " << strerror(errno) << std::endl;
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
		std::cout << FT_STATUS << "Epoll Wait status: " << strerror(errno) << std::endl;
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
            std::cout << FT_EVENT << "Server side event on fd " << FT_HIGH_LIGHT_COLOR << epoll_fd << RESET_COLOR << "." << std::endl;
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
	std::cout << FT_EVENT << "Client " << client_fd << " disconnected." << std::endl;
	_client_server_map.erase(client_fd);
	_request_list.erase(client_fd);
	epoll_ctl(_epoll, EPOLL_CTL_DEL, client_fd, &events_setup);
	close(client_fd);
}

void Network::_handleRecvError(int client_fd, struct epoll_event &events_setup)
{
	std::cout << FT_WARNING << "recv error on client " << client_fd << ": " << strerror(errno) << std::endl;
	_client_server_map.erase(client_fd);
	_request_list.erase(client_fd);
	epoll_ctl(_epoll, EPOLL_CTL_DEL, client_fd, &events_setup);
	close(client_fd);
}

// void Network::recv(int client_fd, struct epoll_event &events_setup)
// {
// 	std::cout
// 		<< FT_EVENT
// 		<< "Recv event happened on fd "
// 		<< FT_HIGH_LIGHT_COLOR << client_fd << RESET_COLOR
// 		<< "." << std::endl;

// 	char client_buffer[FT_DEFAULT_CLIENT_BUFFER_SIZE];
//     std::string& total_request = _request_list[client_fd];
// 	int bytes;

// 	while (sig::keepRunning())
// 	{
// 		bytes = ::recv(client_fd, client_buffer, FT_DEFAULT_CLIENT_BUFFER_SIZE, 0);

// 		if (bytes > 0)
// 		{
// 			std::cout << FT_EVENT << "Receiving " << bytes
// 					  << ((bytes <= 1) ? " byte." : " bytes.")
// 					  << std::endl;
// 			total_request.append(client_buffer, bytes);
// 		}
// 		else if (bytes == 0)
// 		{
// 			_handleClientDisconnect(client_fd, events_setup);
// 			return;
// 		}
// 		else // bytes == -1
// 		{
// 			if (errno == EAGAIN || errno == EWOULDBLOCK)
// 				break; // No more data to read right now
// 			else
// 			{
// 				_handleRecvError(client_fd, events_setup);
// 				return;
// 			}
// 		}
// 	}
// 	if (!total_request.empty() && total_request.find("\r\n\r\n") != std::string::npos)
// 	{
// 		events_setup.data.fd = client_fd;
// 		events_setup.events = EPOLLOUT;
// 		epoll_ctl(_epoll, EPOLL_CTL_MOD, client_fd, &events_setup);
// 	}
// }

void Network::recv(int client_fd, struct epoll_event &events_setup)
{
    std::cout
        << FT_EVENT
        << "Recv event on fd "
        << FT_HIGH_LIGHT_COLOR << client_fd << RESET_COLOR
        << "." << std::endl;

    char client_buffer[FT_DEFAULT_CLIENT_BUFFER_SIZE];
    std::string& total_request = _request_list[client_fd];
    int bytes;

    while (sig::keepRunning())
    {
        bytes = ::recv(client_fd, client_buffer, FT_DEFAULT_CLIENT_BUFFER_SIZE, 0);

        if (bytes > 0)
        {
            std::cout << FT_EVENT << "Receiving " << bytes
                      << ((bytes <= 1) ? " byte." : " bytes.")
                      << std::endl;
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
            {
                break; // all bytes read from this event
            }
            else
            {
                _handleRecvError(client_fd, events_setup);
                return; 
            }
        }
    } 
    if (_isRequestComplete(total_request)) 
    {
        std::cout << FT_OK << "Request complete on fd " << client_fd << ". Switching to Send." << std::endl;
        events_setup.data.fd = client_fd;
        events_setup.events = EPOLLOUT;
        epoll_ctl(_epoll, EPOLL_CTL_MOD, client_fd, &events_setup);
    }
    else
    {
        std::cout << FT_EVENT << "Request incomplete on fd " << client_fd << ". Waiting for more data." << std::endl;
    }
}


bool Network::_isRequestComplete(const std::string& total_request)
{
    size_t header_end = total_request.find("\r\n\r\n");

    if (header_end == std::string::npos)
    {
        return false;//headers incompleteds
    }

    // headers completed
    std::string headers_str = total_request.substr(0, header_end);

    // 2a. Check for Chunked Encoding
    if (headers_str.find("Transfer-Encoding: chunked") != std::string::npos ||
        headers_str.find("transfer-encoding: chunked") != std::string::npos)
    {
        if (total_request.rfind("0\r\n\r\n") != std::string::npos)
            return true; // Body chunked completo!
        // else: Body incompleto.
        return false;
    }
    
    // 2b. Check for Content-Length
    size_t cl_pos = std::string::npos;
    size_t cl_lower = headers_str.find("content-length: ");
    size_t cl_upper = headers_str.find("Content-Length: ");
    if (cl_lower != std::string::npos) cl_pos = cl_lower + 16;
    else if (cl_upper != std::string::npos) cl_pos = cl_upper + 16;

    if (cl_pos != std::string::npos)
    {
        // Encontrou Content-Length.
        std::stringstream ss(headers_str.substr(cl_pos));
        size_t content_length = 0;
        ss >> content_length;
        size_t body_length = total_request.length() - (header_end + 4);   
        if (body_length >= content_length) {
            return true; // Body completo recebido!
        }
        // else: Body incompleto.
        return false;
    }
    return true;
}
/* ... (includes, construtor, destrutor, recv, _isRequestComplete, etc.) ... */
/* ... (tudo igual até aqui) ... */

void Network::send(int client_fd, struct epoll_event &events_setup)
{
    std::cout
        << FT_EVENT
        << "Send event on fd "
        << FT_HIGH_LIGHT_COLOR << client_fd << RESET_COLOR
        << "." << std::endl;

    bool should_close = true; // Default é fechar em caso de erro

    try 
    {
        if (_request_list.find(client_fd) == _request_list.end()) {
             throw std::runtime_error("No request data for client.");
        }
        if (_client_server_map.find(client_fd) == _client_server_map.end())
            throw std::runtime_error("Client FD not mapped to any server socket.");
        
        // 1. Processar
        Request request(_request_list[client_fd]);
        Response response = Router::dispatchRequest(_config, request, _client_server_map.at(client_fd)->getHostPortPair());
        
        // 2. Decidir (Lógica de Keep-Alive movida)
        should_close = _shouldCloseConnection(request, response);

        // 3. Enviar
        std::string msg = response.stringify();
        ssize_t ret = ::send(client_fd, msg.data(), msg.length(), 0);
        if (ret == -1)
            throw std::runtime_error(std::string("Send failed: ") + strerror(errno));

        // 4. Gerir a conexão (Lógica de fechar/manter movida)
        _manageConnection(client_fd, should_close, events_setup);
    }
    catch (std::exception& e)
    {
        // O 'catch' agora só precisa de fechar
        std::cout << FT_STATUS << "Error during send/dispatch: " << e.what() << ". Forcing close." << std::endl;
        _manageConnection(client_fd, true, events_setup); // Força o fecho
    }
}


// --- NOVAS FUNÇÕES HELPER ---

bool Network::_shouldCloseConnection(const Request& request, const Response& response)
{
    // --- LÓGICA KEEP-ALIVE ---
    std::string connection_header;

    // Regra 1: O cliente pediu para fechar?
    const std::map<std::string, std::string>& reqHeaders = request.getHeaders();
    std::map<std::string, std::string>::const_iterator it = reqHeaders.find("Connection");
    
    if (it == reqHeaders.end()) {
        it = reqHeaders.find("connection"); // Tenta minúsculo
    }
    
    if (it != reqHeaders.end()) {
        connection_header = it->second; // Encontrou o header
        if (connection_header == "close") {
            return true; // Cliente pediu para fechar
        }
    }

    // Regra 2: É HTTP/1.0? (O default é 'close' a menos que peçam 'keep-alive')
    if (request.getVersion() == "HTTP/1.0" && connection_header != "keep-alive") {
        return true;
    }

    // Regra 3: O servidor decidiu fechar?
    const std::map<std::string, std::string>& respHeaders = response.getHeaders();
    std::map<std::string, std::string>::const_iterator resp_it = respHeaders.find("Connection");

    if (resp_it == respHeaders.end()) {
        resp_it = respHeaders.find("connection");
    }

    if (resp_it != respHeaders.end() && resp_it->second == "close") {
        return true; 
    }
    
    return false; // Default: keep-alive
}

void Network::_manageConnection(int client_fd, bool should_close, struct epoll_event &events_setup)
{
    _request_list.erase(client_fd); 

    if (should_close)
    {
        std::cout << FT_EVENT << "Connection: close. Closing fd " << client_fd << "." << std::endl;
        _client_server_map.erase(client_fd);
        epoll_ctl(_epoll, EPOLL_CTL_DEL, client_fd, &events_setup);
        close(client_fd);
    }
    else
    {
        std::cout << FT_EVENT << "Connection: keep-alive. Resetting fd " << client_fd << " to Recv." << std::endl;
        events_setup.data.fd = client_fd;
        events_setup.events = EPOLLIN;
        epoll_ctl(_epoll, EPOLL_CTL_MOD, client_fd, &events_setup);
    }
}


/* ... (Resto do ficheiro, como a sobrecarga do operador << e as exceções) ... */
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
    os << "--- Network Debug Status ---\n";
    os << "- Epoll FD: " << rhs.getEpollFd() << "\n";
    os << "- Listening Sockets: " << rhs.getConnections().size() << "\n";
    const std::vector<Socket*>& sockets = rhs.getConnections();
    for (size_t i = 0; i < sockets.size(); ++i)
    {
        os << "  - Socket " << i << " (FD: " << sockets[i]->getSock() << ") on "
           << sockets[i]->getHostPortPair().getHost() << ":"
           << sockets[i]->getHostPortPair().getPort() << "\n";
    }
    os << "- Pending Requests (waiting send): " << rhs.getRequestList().size() << "\n";
    os << "- Active Client Connections: " << rhs.getClientServerMap().size() << "\n";

    os << "------------------------------" << std::endl;
    return os;
}

const char *Network::EpollException::what() const throw()
{
	return (FT_ERROR "Can't create epoll.");
}

const char *Network::EpollCtlException::what() const throw()
{
	return (FT_ERROR "Can't manage file descriptor.");
}

const char *Network::EpollWaitException::what() const throw()
{
	return (FT_ERROR "Can't wait events.");
}