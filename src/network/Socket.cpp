#include "network/Socket.hpp"

Socket::Socket(HostPortPair const& listenPair)
: _listenOn(listenPair)
{
	Log::dev("setup", "Setting up socket on " + Log::hl(_listenOn) + ".");

	_loadAddressInfo();
	_createSocket();

	Log::dev("setup", "Socket created on " + Log::hl(_listenOn) + ".");
}

Socket::~Socket()
{
	close(_sock);

	Log::dev("setup", "Freeing Address Info memory...");
	if (_servinfo) {
		freeaddrinfo(_servinfo);
		_servinfo = NULL;
	}

	Log::prod("ok", "Socket " + Log::hl(_listenOn) + " closed.");
}

void	Socket::_setAddrStruct()
{
	std::memset(&_hints, 0, sizeof(_hints));
	_hints.ai_family = AF_UNSPEC;	  // IPv4 only
	_hints.ai_socktype = SOCK_STREAM; // TCP or UDP
	_hints.ai_flags = AI_PASSIVE;	  // allows bind
}

void	Socket::_loadAddressInfo()
{
	Log::dev("setup", "Loading address info...");
	int status;
	_setAddrStruct();

	// Convert size_t (port) to C-string
	std::string portStr = utils::str(_listenOn.getPort());

	// Get host. If it's "0.0.0.0" or "*", we use NULL for getaddrinfo
	std::string hostStr = _listenOn.getHost();
	char const* host = (hostStr == "*" || hostStr == "0.0.0.0") ? NULL : hostStr.c_str();

	// Use the values from _listenOn
	status = getaddrinfo(host, portStr.c_str(), &_hints, &_servinfo);

	if (status) {
		Log::prod("status", "Address info status: " + utils::str(gai_strerror(status)));
		throw GetAddrInfoException();
	}
}

void	Socket::_createSocket()
{
	_sock = socket(_servinfo->ai_family, _servinfo->ai_socktype, _servinfo->ai_protocol);

	Log::dev("setup", "Creating Socket...");
	if (_sock < 0) {
		throw SocketException();
	}

	Log::dev("setup", "Configuring Socket...");
	int yes = 1;
	if (setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
		throw SetSockOptException();
	}

	Log::dev("setup", "Setting Socket to non-blocking mode...");
	int flags = fcntl(_sock, F_GETFL);
    int fdflags = fcntl(_sock, F_GETFD);
    if (flags == -1 || fdflags == -1
		|| fcntl(_sock, F_SETFL, flags | O_NONBLOCK) == -1
		|| fcntl(_sock, F_SETFD, fdflags | FD_CLOEXEC) == -1) {
		throw FcntlException();
    }
}

int	Socket::getSock()
{
	return (_sock);
}

void	Socket::bind()
{
	int status;

	Log::dev("setup", "Binding socket on " + Log::hl(_listenOn) + "...");

	status = ::bind(_sock, _servinfo->ai_addr, _servinfo->ai_addrlen);

	if (status < 0) {
		Log::prod("status",  "Bind status: " + utils::str(strerror(errno)));
		throw BindException();
	}
}

void	Socket::listen()
{
	int status;

	Log::dev("setup", "Putting Socket " + Log::hl(_listenOn) + " in Listen Mode...");

	status = ::listen(_sock, 10);
	if (status < 0) {
		Log::prod("status", "Listen status: " + utils::str(strerror(errno)));
		throw ListenException();
	}
	Log::prod("ok", "Now listening on " + Log::hl(_listenOn) + ".");
}

int	Socket::accept()
{
	struct sockaddr_storage theirAddr;
	socklen_t addrSize;
	int newSocket;

	Log::dev("event", "accept() requested.");
	addrSize = sizeof theirAddr;
	newSocket = ::accept(_sock, (struct sockaddr *)&theirAddr, &addrSize);
	if (newSocket < 0) {
		Log::prod("error", "accept(): " + utils::str(strerror(errno)));
		throw AcceptException();
	}
	Log::dev("ok", "accept() successfull");
	return (newSocket);
}

HostPortPair const&	Socket::getHostPortPair() const
{
    return (_listenOn);
}

char const*	Socket::GetAddrInfoException::what() const throw()
{
	return ("Can't get Address's info.");
}

char const*	Socket::SocketException::what() const throw()
{
	return ("Can't create Socket.");
}

char const*	Socket::SetSockOptException::what() const throw()
{
	return ("Can't configure Socket.");
}

char const*	Socket::FcntlException::what() const throw()
{
	return ("Can't set Socket to non blocking mode.");
}

char const*	Socket::BindException::what() const throw()
{
	return ("Can't bind socket with IP and port.");
}

char const*	Socket::ListenException::what() const throw()
{
	return ("Can't put Socket in Listen Mode.");
}

char const*	Socket::AcceptException::what() const throw()
{
	return ("Accept rejected.");
}

char const*	Socket::ConnectException::what() const throw()
{
	return ("Connect rejected.");
}