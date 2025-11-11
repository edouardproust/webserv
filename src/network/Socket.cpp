#include "network/Socket.hpp"

Socket::Socket(const HostPortPair &listen_pair) : _listenOn(listen_pair)
{
	if (DEVMODE) std::cout << FT_SETUP << "Setting up socket on " << FT_HIGH_LIGHT_COLOR << _listenOn << RESET_COLOR << "." << std::endl;

	loadAddressInfo();
	createSocket();

	if (DEVMODE) std::cout << FT_OK << "Socket created on " << FT_HIGH_LIGHT_COLOR << _listenOn  << RESET_COLOR << "." << std::endl;
}

Socket::~Socket()
{
	close(_sock);
	std::cout << FT_OK << "Socket " << FT_HIGH_LIGHT_COLOR << _listenOn << RESET_COLOR << " closed." << std::endl;
}

void Socket::setAddrStruct(void)
{
	std::memset(&_hints, 0, sizeof(_hints));
	_hints.ai_family = AF_UNSPEC;	  // IPv4 only // TODO support IPv6 ?
	_hints.ai_socktype = SOCK_STREAM; // TCP or UDP
	_hints.ai_flags = AI_PASSIVE;	  // allows bind
}

void Socket::loadAddressInfo()
{
	if (DEVMODE) std::cout << FT_SETUP << "Loading address info..." << std::endl;
	int status;
	setAddrStruct();

	// Convert size_t (port) to C-string
	std::string portStr = utils::toString(_listenOn.getPort());

	// Get host. If it's "0.0.0.0" or "*", we use NULL for getaddrinfo
	std::string hostStr = _listenOn.getHost();
	const char* host = (hostStr == "*" || hostStr == "0.0.0.0") ? NULL : hostStr.c_str(); //TODO

	// Use the values from _listenOn
	status = getaddrinfo(host, portStr.c_str(), &_hints, &_servinfo);

	if (status) {
		std::cout << FT_STATUS << "Address info status: " << gai_strerror(status) << std::endl;
		throw GetAddrInfoException();
	}
}

void Socket::createSocket()
{
	_sock = ::socket(_servinfo->ai_family, _servinfo->ai_socktype, _servinfo->ai_protocol);

	if (DEVMODE) std::cout << FT_SETUP << "Creating Socket..." << std::endl;
	if (_sock < 0) {
		::freeaddrinfo(_servinfo);
		throw SocketException();
	}

	int yes = 1;

	if (DEVMODE) std::cout << FT_SETUP << "Configuring Socket..." << std::endl;
	if (::setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
		::freeaddrinfo(_servinfo);
		throw SetSockOptException();
	}

	if (DEVMODE) std::cout << FT_SETUP << "Setting Socket to non-blocking mode..." << std::endl;
	int status = ::fcntl(_sock, F_SETFL, O_NONBLOCK, FD_CLOEXEC);
	if (status < 0) {
		::freeaddrinfo(_servinfo);
		throw FcntlException();
	}
}

int Socket::getSock()
{
	return (_sock);
}

void Socket::bind()
{
	int status;

	if (DEVMODE) std::cout << FT_SETUP << "Binding socket on " << FT_HIGH_LIGHT_COLOR << _listenOn << RESET_COLOR << "..." << std::endl;

	status = ::bind(_sock, _servinfo->ai_addr, _servinfo->ai_addrlen);

	if (status < 0) {
		freeaddrinfo(_servinfo);
		std::cout << FT_STATUS << "Bind status: " << strerror(errno) << std::endl;
		throw BindException();
	}

	if (DEVMODE) std::cout << FT_SETUP << "Freeing Address Info memory..." << std::endl;
	freeaddrinfo(_servinfo);
}

void Socket::listen()
{
	int status;

	if (DEVMODE) std::cout << FT_SETUP << "Putting Socket " << FT_HIGH_LIGHT_COLOR << _listenOn << RESET_COLOR << " in Listen Mode..." << std::endl;

	status = ::listen(_sock, 10);
	if (status < 0) {
		std::cout << FT_STATUS << "Listen status: " << strerror(errno) << std::endl;
		throw ListenException();
	}
	std::cout << FT_OK << "Now listening on " << FT_HIGH_LIGHT_COLOR << _listenOn << RESET_COLOR << "." << std::endl;
}

int Socket::accept()
{
	struct sockaddr_storage their_addr;
	socklen_t addr_size;
	int new_socket;

	std::cout << FT_EVENT << "Accept requested." << std::endl;
	addr_size = sizeof their_addr;
	new_socket = ::accept(_sock, (struct sockaddr *)&their_addr, &addr_size);
	if (new_socket < 0) {
		std::cout << FT_STATUS << "Accept status: " << strerror(errno) << std::endl;
		throw AcceptException();
	}
	std::cout << FT_OK << "Accept successfull!" << std::endl;
	return (new_socket);
}

const HostPortPair& Socket::getHostPortPair() const
{
    return (_listenOn);
}

const char *Socket::GetAddrInfoException::what() const throw()
{
	return (FT_ERROR "Can't get Address's info.");
}

const char *Socket::SocketException::what() const throw()
{
	return (FT_ERROR "Can't create Socket.");
}

const char *Socket::SetSockOptException::what() const throw()
{
	return (FT_ERROR "Can't configure Socket.");
}

const char *Socket::FcntlException::what() const throw()
{
	return (FT_ERROR "Can't set Socket to non blocking mode.");
}

const char *Socket::BindException::what() const throw()
{
	return (FT_ERROR "Can't bind socket with IP and port.");
}

const char *Socket::ListenException::what() const throw()
{
	return (FT_ERROR "Can't put Socket in Listen Mode.");
}

const char *Socket::AcceptException::what() const throw()
{
	return (FT_ERROR "Accept rejected.");
}

const char *Socket::ConnectException::what() const throw()
{
	return (FT_ERROR "Connect rejected.");
}