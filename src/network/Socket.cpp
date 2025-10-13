#include "network/Socket.hpp"

Socket::Socket(const t_server_config &server) : _server(server), _servinfo(0), _sock(-1)
{
    std::cout << FT_SETUP << "Setting up " << FT_HIGH_LIGHT_COLOR << this->_server.server_names[0] << RESET_COLOR
              << " socket on port " << FT_HIGH_LIGHT_COLOR << this->_server.port << RESET_COLOR << "." << std::endl;

    setAddrStruct();
    loadAddressInfo();
    createSocket();

    std::cout << FT_OK << FT_HIGH_LIGHT_COLOR << this->_server.server_names[0] << RESET_COLOR
              << " socket on port " << FT_HIGH_LIGHT_COLOR << this->_server.port << RESET_COLOR
              << " is ready." << std::endl;
    std::cout << std::endl;
}

Socket::~Socket()
{
    std::cout << FT_CLOSE << "Closing " << FT_HIGH_LIGHT_COLOR << _server.server_names[0] << RESET_COLOR
              << " socket on port " << FT_HIGH_LIGHT_COLOR << _server.port << RESET_COLOR << "." << std::endl;

    if (_sock >= 0)
        close(_sock);
    if (_servinfo)
        freeaddrinfo(_servinfo);
}

void Socket::setAddrStruct(void)
{
    std::memset(&_hints, 0, sizeof(_hints));
    _hints.ai_family = AF_UNSPEC;      /* IPv4 or IPv6 */
    _hints.ai_socktype = SOCK_STREAM;  /* TCP */
    _hints.ai_flags = AI_PASSIVE;      /* for bind() */
}

void Socket::loadAddressInfo(void)
{
    std::cout << FT_SETUP << "Loading address info." << std::endl;
    int status = getaddrinfo(NULL, _server.port.c_str(), &_hints, &_servinfo);
    if (status != 0)
    {
        std::cout << FT_STATUS << "getaddrinfo: " << gai_strerror(status) << std::endl;
        throw std::runtime_error("getaddrinfo failed");
    }
    std::cout << FT_OK << "Address info loaded." << std::endl;
}

void Socket::createSocket(void)
{
    std::cout << FT_SETUP << "Creating socket." << std::endl;
    _sock = ::socket(_servinfo->ai_family, _servinfo->ai_socktype, _servinfo->ai_protocol);
    if (_sock < 0)
        throw std::runtime_error("socket() failed");
    std::cout << FT_OK << "Socket created." << std::endl;

    int yes = 1;
    if (setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
        throw std::runtime_error("setsockopt failed");

    int flags = fcntl(_sock, F_GETFL, 0);
    if (flags == -1)
        throw std::runtime_error("fcntl(F_GETFL) failed");
    if (fcntl(_sock, F_SETFL, flags | O_NONBLOCK) == -1)
        throw std::runtime_error("fcntl(F_SETFL) failed");
    if (fcntl(_sock, F_SETFD, FD_CLOEXEC) == -1)
        throw std::runtime_error("fcntl(F_SETFD) failed");

    std::cout << FT_OK << "Socket configured (non-blocking, CLOEXEC)." << std::endl;
}

int Socket::getSock(void) const
{
    return _sock;
}

void Socket::bindSocket(void)
{
    std::cout << FT_SETUP << "Binding socket on port " << FT_HIGH_LIGHT_COLOR << _server.port << RESET_COLOR << "." << std::endl;
    if (::bind(_sock, _servinfo->ai_addr, _servinfo->ai_addrlen) < 0)
    {
        std::cout << FT_STATUS << "bind: " << strerror(errno) << std::endl;
        throw std::runtime_error("bind() failed");
    }
    freeaddrinfo(_servinfo);
    _servinfo = 0;
    std::cout << FT_OK << "Bind successful." << std::endl;
}

void Socket::listenSocket(void)
{
    std::cout << FT_SETUP << "Listening on socket." << std::endl;
    if (::listen(_sock, 128) < 0)
    {
        std::cout << FT_STATUS << "listen: " << strerror(errno) << std::endl;
        throw std::runtime_error("listen() failed");
    }
    std::cout << FT_OK << "Listening." << std::endl;
}

int Socket::acceptConnection(void)
{
    struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof(their_addr);
    int new_socket = ::accept(_sock, (struct sockaddr *)&their_addr, &addr_size);
    if (new_socket < 0)
        throw std::runtime_error(std::string("accept() failed: ") + strerror(errno));
    return new_socket;
}