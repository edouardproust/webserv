#include <network/Network.hpp>
#include <network/Colors.hpp>
#include <config/Config.hpp>
#include <config/ServerBlock.hpp> // <--- ADICIONE
#include <config/HostPortPair.hpp> // <--- ADICIONE
#include "http/Request.hpp"
#include "http/Response.hpp"
#include "http/RequestParser.hpp" // Para enums de erro
#include "router/Router.hpp"  
#include <set> // <--- ADICIONE
#include <errno.h>
#include <cstring>
#include <sstream>

Network::Network(Config const& _config_file) : _config_file(_config_file)
{
	// Initializes Ctrl + C signal handler
	keep();

	std::cout
		<< FT_SETUP
		<< "Setting up Web server from "
		<< FT_HIGH_LIGHT_COLOR << "meu cu" << RESET_COLOR //TODO
		<< " file."
		<< std::endl;
	
    // 1. Colete todos os HostPortPair únicos de todos os ServerBlocks
    std::set<HostPortPair> unique_listens;
    const std::vector<ServerBlock>& servers = _config_file.getServers(); //
    
    for (size_t i = 0; i < servers.size(); ++i)
    {
        const std::set<HostPortPair>& listens = servers[i].getListen(); //
        unique_listens.insert(listens.begin(), listens.end());
    }

    // 2. Crie um Socket para cada HostPortPair único
    for (std::set<HostPortPair>::const_iterator it = unique_listens.begin(); 
         it != unique_listens.end() && keep(); ++it)
    {
        _connections.push_back(new Socket(*it)); // Usa o novo construtor do Socket
    }

	// 3. Faça Bind e Listen de todos os sockets criados
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
			// Limpeza em caso de falha
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
                // Mapeie o novo client_fd ao Socket* que o aceitou
                _client_server_map[new_conn] = *it; 
            }
			return (new_conn);
		}
			
	}
	return (0);
}

std::string Network::getBoundry(std::string request)
{
	size_t start;
	size_t end;

	start = request.find("boundary=", 0) + 9;
	if (start == std::string::npos)
		return ("");
	end = request.find("\r", start);
	return (request.substr(start, end - start));
}

int  Network::getRequestTotalLength(std::string request)
{
	size_t	start;
	size_t	end;
	int		length;
	std::string boundry;

	boundry = getBoundry(request);
	if (boundry == "")
		return (request.length());

	length = request.rfind(boundry, request.length()) - 2;
	start = request.find("Content-Length: ", 0);
	if (start == std::string::npos)
		return (request.length());
	start += 16;
	end = request.find("\r", start);
	length += std::atoi(request.substr(start, end - start).data());
	return (length);
}

void Network::recv(int client_fd, struct epoll_event &events_setup)
{
	std::cout
		<< FT_EVENT
		<< "Recv event happened on fd "
		<< FT_HIGH_LIGHT_COLOR << client_fd << RESET_COLOR
		<< "." << std::endl;

	char client_buffer[FT_DEFAULT_CLIENT_BUFFER_SIZE];
	std::string total_request;
	int total_bytes = -1;
	int bytes = 0;
	int current_bytes_read = -2;

	std::memset(client_buffer, 0, FT_DEFAULT_CLIENT_BUFFER_SIZE);
	while (current_bytes_read < total_bytes && keep())
	{
		bytes = ::recv(client_fd, client_buffer, sizeof(client_buffer), 0);
		if (bytes == 0)
			break ;
		if (bytes == -1)
			break ;
		if (total_bytes == -1)
		{
			current_bytes_read = 0;
			total_bytes = getRequestTotalLength(client_buffer);
		}
		if (bytes > 0)
		{
			std::cout << FT_EVENT << "Receiving " << bytes
					  << ((bytes <= 1) ? " byte." : " bytes.")
					  << std::endl;
			current_bytes_read += bytes;
			total_request.append(client_buffer, bytes);
		}
	}
	std::cout << FT_EVENT << total_request.length()
			  << ((total_request.length() <= 1) ? " byte" : " bytes")
			  << " received."
			  << " recv exited with " << bytes << " bytes."
			  << std::endl;

	if (bytes == -1)
		std::cout << FT_WARNING << "Failed to receive full request from client [" << client_fd << "]" << std::endl;
		
	else if (total_request.length() > 0)
	{
		_request_list[client_fd] = total_request;
		events_setup.data.fd = client_fd;
		events_setup.events = EPOLLOUT;
		epoll_ctl(_epoll, EPOLL_CTL_MOD, client_fd, &events_setup);
		return ;
	}
	_client_server_map.erase(client_fd);
	events_setup.data.fd = client_fd;
	epoll_ctl(_epoll, EPOLL_CTL_DEL, client_fd, &events_setup);
	close(client_fd);
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
        // --- INÍCIO DA MUDANÇA ---
        // Em vez de:
        // Request request;
        // request.parse(_request_list[client_fd]);
        //
        // Use o construtor que faz o parse imediatamente:
        Request request(_request_list[client_fd]); //
        // --- FIM DA MUDANÇA ---


        // 3. Encontre o HostPortPair de origem
        if (_client_server_map.find(client_fd) == _client_server_map.end())
            throw std::runtime_error("Network logic error: client_fd not in map.");
        
        Socket* serverSocket = _client_server_map.at(client_fd);
        HostPortPair listenPair = serverSocket->getHostPortPair(); // (Do nosso refactor do Socket)

        // 4. Chame o Router (ele próprio já trata dos erros de parse)
        // (Como visto no Router.cpp, ele verifica o request.getStatus())
        response = Router::dispatchRequest(_config_file, request, listenPair);
    
        // 5. Converta a Response para string (do Response.cpp)
		std::string msg = response.stringify();

        // 6. Envie
		int ret = ::send(client_fd, msg.data(), msg.length(), 0);
		if (ret == -1)
			throw std::runtime_error("Send failed");

        // 7. Limpeza (em caso de sucesso)
		_request_list.erase(client_fd);
        _client_server_map.erase(client_fd);
		events_setup.data.fd = client_fd;
		epoll_ctl(_epoll, EPOLL_CTL_DEL, client_fd, &events_setup);
		close(client_fd);
	}
	catch (std::exception& e)
	{
		std::cout << FT_STATUS << "Error during send/dispatch: " << e.what() << std::endl;
        
        // 8. Limpeza (em caso de falha)
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
