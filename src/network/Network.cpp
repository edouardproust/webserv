#include <network/Network.hpp>
#include <network/Colors.hpp>
#include <config/Config.hpp>
#include <config/ServerBlock.hpp> 
#include <config/HostPortPair.hpp> 
#include "http/Request.hpp"
#include "http/Response.hpp"
#include "http/RequestParser.hpp" 
#include "router/Router.hpp"  
#include <set> 
#include <errno.h>
#include <cstring>
#include <sstream>

Network::Network(Config const& _config_file) : _config_file(_config_file)
{
	// Initializes Ctrl + C signal handler
	keep();
    //  GET HoSTpORTpAIr
    std::set<HostPortPair> unique_listens;
    const std::vector<ServerBlock>& servers = _config_file.getServers(); //
    
    for (size_t i = 0; i < servers.size(); ++i)
    {
        const std::set<HostPortPair>& listens = servers[i].getListen(); //
        unique_listens.insert(listens.begin(), listens.end());
    }

    //  create socket for each HostPortPair
    for (std::set<HostPortPair>::const_iterator it = unique_listens.begin(); 
         it != unique_listens.end() && keep(); ++it)
    {
        _connections.push_back(new Socket(*it)); // Usa o novo construtor do Socket
    }

	//  bind and listen to each socket created
	std::vector<Socket *>::iterator it_sock;
	for (it_sock = _connections.begin(); it_sock != _connections.end(); it_sock++)
	{
		try
		{
			(*it_sock)->bind();
			(*it_sock)->listen();
		}
		catch (Socket::BindException& e)
		{
			// Cleanup allocated sockets before throwing
			for (std::vector<Socket *>::iterator it_del = _connections.begin(); it_del != _connections.end(); it_del++)
				delete *it_del;
			throw e;
		}
		std::cout << std::endl;
	}
}

Network::~Network()
{
	std::cout << FT_CLOSE << "Closing Web server." << std::endl;

	std::vector<Socket *>::iterator it;

	std::cout << FT_CLOSE << "Freeing Socket memory." << std::endl;
	for (it = _connections.begin(); it != _connections.end(); it++)
		delete *it;

	std::cout << FT_OK << "Socket memory is now free!" << std::endl;

	std::cout << FT_CLOSE << "Closing epoll." << std::endl;
	close(_epoll);
	std::cout << FT_OK << "Epoll closed!" << std::endl;

	std::cout << FT_OK << "Web server closed!" << std::endl;
	std::cout << std::endl;
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
	std::vector<Socket *>::iterator it;

	events_setup.events = EPOLLIN | EPOLLOUT;
	for (it = _connections.begin(); it != _connections.end(); it++)
	{
		events_setup.data.fd = (*it)->getSock();
		if (::epoll_ctl(_epoll, EPOLL_CTL_ADD, (*it)->getSock(), &events_setup) < 0)
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
	if (nfds == -1 && keep())
	{
		std::cout << FT_STATUS << "Epoll Wait status: " << strerror(errno) << std::endl;
		throw EpollWaitException();
	}

	return (nfds);
}

int Network::isServerSideEvent(int epoll_fd)
{
	std::vector<Socket *>::iterator it;

	std::cout
		<< FT_EVENT
		<< "Server side event happened on fd "
		<< FT_HIGH_LIGHT_COLOR << epoll_fd << RESET_COLOR
		<< "." << std::endl;

	for (it = _connections.begin(); it != _connections.end() && keep(); it++)
	{
		if (epoll_fd == (*it)->getSock())
		{
			int new_conn = (*it)->accept();
            if (new_conn > 0)
            {
               // Map the new client fd to the server Socket* that accepted it
                _client_server_map[new_conn] = *it; 
            }
			return (new_conn);
		}
			
	}
	return (0);
}

// std::string Network::getBoundry(std::string request)
// {
// 	size_t start;
// 	size_t end;

// 	start = request.find("boundary=", 0) + 9;
// 	if (start == std::string::npos)
// 		return ("");
// 	end = request.find("\r", start);
// 	return (request.substr(start, end - start));
// }

// int  Network::getRequestTotalLength(std::string request)
// {
// 	size_t	start;
// 	size_t	end;
// 	int		length;
// 	std::string boundry;

// 	boundry = getBoundry(request);
// 	if (boundry == "")
// 		return (request.length());

// 	length = request.rfind(boundry, request.length()) - 2;
// 	start = request.find("Content-Length: ", 0);
// 	if (start == std::string::npos)
// 		return (request.length());
// 	start += 16;
// 	end = request.find("\r", start);
// 	length += std::atoi(request.substr(start, end - start).data());
// 	return (length);
// }

// Em Network.cpp


void Network::recv(int client_fd, struct epoll_event &events_setup)
{
	std::cout
		<< FT_EVENT
		<< "Recv event happened on fd "
		<< FT_HIGH_LIGHT_COLOR << client_fd << RESET_COLOR
		<< "." << std::endl;

	char client_buffer[FT_DEFAULT_CLIENT_BUFFER_SIZE + 1]; // +1 para null terminator
	
    // Pega uma referência ao request string (ou cria um novo vazio)
    std::string& total_request = _request_list[client_fd];
	int bytes;

	while (keep())
	{
		std::memset(client_buffer, 0, sizeof(client_buffer));
		bytes = ::recv(client_fd, client_buffer, FT_DEFAULT_CLIENT_BUFFER_SIZE, 0);

		if (bytes > 0)
		{
            // receivng data
			std::cout << FT_EVENT << "Receiving " << bytes
					  << ((bytes <= 1) ? " byte." : " bytes.")
					  << std::endl;
			total_request.append(client_buffer, bytes);
		}
		else if (bytes == 0)
		{
            // client disconnected
			std::cout << FT_EVENT << "Client " << client_fd << " disconnected." << std::endl;
			_client_server_map.erase(client_fd);
			_request_list.erase(client_fd); // clean up the request
			epoll_ctl(_epoll, EPOLL_CTL_DEL, client_fd, &events_setup);
			close(client_fd);
			return;
		}
		else // bytes == -1
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break; // No more data to read right now
			else
			{
				// error occurred
				std::cout << FT_WARNING << "recv error on client " << client_fd << ": " << strerror(errno) << std::endl;
				_client_server_map.erase(client_fd);
				_request_list.erase(client_fd); 
				epoll_ctl(_epoll, EPOLL_CTL_DEL, client_fd, &events_setup);
				close(client_fd);
				return; 
			}
		}
	} 
	if (!total_request.empty()) // not empty change to EPOLLOUT to send response
	{
		events_setup.data.fd = client_fd;
		events_setup.events = EPOLLOUT;
		epoll_ctl(_epoll, EPOLL_CTL_MOD, client_fd, &events_setup);
	}
}

void Network::send(int client_fd, struct epoll_event &events_setup)
{
	std::cout
		<< FT_EVENT
		<< "Send event happened on fd "
		<< FT_HIGH_LIGHT_COLOR << client_fd << RESET_COLOR
		<< "." << std::endl;

	Response response;
	try 
	{
        Request request(_request_list[client_fd]); 
        if (_client_server_map.find(client_fd) == _client_server_map.end())
            throw std::runtime_error("Network logic error: client_fd not in map.");
        Socket* serverSocket = _client_server_map.at(client_fd);
        HostPortPair listenPair = serverSocket->getHostPortPair(); 
		//call router
        response = Router::dispatchRequest(_config_file, request, listenPair);
		std::string msg = response.stringify();
		int ret = ::send(client_fd, msg.data(), msg.length(), 0);
		if (ret == -1)
			throw std::runtime_error("Send failed");
		_request_list.erase(client_fd);
        _client_server_map.erase(client_fd);
		events_setup.data.fd = client_fd;
		epoll_ctl(_epoll, EPOLL_CTL_DEL, client_fd, &events_setup);
		close(client_fd);
	}
	catch (std::exception& e)
	{
		std::cout << FT_STATUS << "Error during send/dispatch: " << e.what() << std::endl;
        _request_list.erase(client_fd);
        _client_server_map.erase(client_fd);
		epoll_ctl(_epoll, EPOLL_CTL_DEL, client_fd, &events_setup);
		close(client_fd);
	}
}

void Network::start_servers()
{
	std::cout << FT_SETUP << "Starting up Web server" << std::endl;
	std::cout << FT_WARNING << "Press Ctrl + C to stop the Web server." << std::endl;

	int number_of_events;
	int new_conn;
	struct epoll_event events[FT_MAX_EVENT_SIZE];
	struct epoll_event events_setup;

	epoll();
	epollAddServers();

	while (keep())
	{
		number_of_events = epoll_wait(events);
		if (number_of_events > 0)
        {
            std::cout << "\n--- SERVER STATUS (Activity Detected) ---\n" << *this << "\n";
        }
		for (int n = 0; n < number_of_events && keep(); n++)
		{
			if ((new_conn = isServerSideEvent(events[n].data.fd)) != 0)
			{
				::fcntl(new_conn, F_SETFL, O_NONBLOCK, FD_CLOEXEC);
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

const char *Network::cantGetUserInfoException::what() const throw()
{
	return (FT_ERROR "Can't get user information.");
}

const char *Network::needSudoException::what() const throw()
{
	return (FT_ERROR "Sudo required.");
}
