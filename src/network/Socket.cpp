#include "network/Socket.hpp"

Socket::Socket(HostPortPair const& listenPair)
: _listenDirective(listenPair)
{
	_loadAddressInfo();
	_createSocket();
	Log::dev("setup", "New socket created as non-blocking (fd " + Log::hl(_fd) + ").");
}

Socket::~Socket()
{
	close(_fd);
	if (_servinfo) {
		freeaddrinfo(_servinfo);
		_servinfo = NULL;
	}
	Log::prod("close", "Socket closed (fd " + Log::hl(_fd) + " on " + Log::hl(_listenDirective) + ").");
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
	_setAddrStruct();

	// Convert size_t (port) to C-string
	std::string portStr = utils::str(_listenDirective.getPort());

	// Get host. If it's "0.0.0.0" or "*", we use NULL for getaddrinfo
	std::string hostStr = _listenDirective.getHost();
	char const* host = (hostStr == "*" || hostStr == "0.0.0.0") ? NULL : hostStr.c_str();

	// Use the values from _listenDirective
	int status = getaddrinfo(host, portStr.c_str(), &_hints, &_servinfo);

	if (status) {
		Log::prod("status", "Address info status: " + utils::str(gai_strerror(status)));
		throw std::runtime_error("Can't get Address's info.");
	}
}

void	Socket::_createSocket()
{
	_fd = socket(_servinfo->ai_family, _servinfo->ai_socktype, _servinfo->ai_protocol);
	//Log::dev("setup", "Creating Socket...");
	if (_fd < 0) {
		throw std::runtime_error("Can't create new socket.");
	}
	//Log::dev("setup", "Configuring Socket...");
	int yes = 1;
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
		throw std::runtime_error("Can't configure socket (fd " + Log::hl(_fd) + ").");
	}
	//Log::dev("setup", "Setting Socket to non-blocking mode...");
	int flags = fcntl(_fd, F_GETFL);
    int fdflags = fcntl(_fd, F_GETFD);
    if (flags == -1 || fdflags == -1
		|| fcntl(_fd, F_SETFL, flags | O_NONBLOCK) == -1
		|| fcntl(_fd, F_SETFD, fdflags | FD_CLOEXEC) == -1) {
		throw std::runtime_error("Can't set socket (fd " + Log::hl(_fd) + ") to non blocking mode.");
    }
}

int	Socket::getFd() const
{
	return (_fd);
}

void	Socket::safeBind()
{
	int status = bind(_fd, _servinfo->ai_addr, _servinfo->ai_addrlen);
	if (status < 0) {
		throw std::runtime_error("Couldn't bind socket (fd " + Log::hl(_fd) + ") with address " + Log::hl(_listenDirective) + ": " + utils::str(strerror(errno)) + ".");
	}
}

void	Socket::safeListen()
{
	int status = listen(_fd, 10);
	if (status < 0) {
		Log::prod("status", "Listen error: " + utils::str(strerror(errno)));
		throw std::runtime_error("Can't put Socket in Listen Mode.");
	}
	Log::dev("setup", "Socket (fd " + Log::hl(_fd) + ") is now listening on " + Log::hl(_listenDirective) + ".");
}

/**
 * Accepts a new client connection on this listening socket.
 * Returns new client socket's fd (or -1 in case of accept() error).
 */
int	Socket::createNewClientSocket()
{
	struct sockaddr_storage clientInfos;

	socklen_t clientInfoSize = sizeof(clientInfos);
	int newClientSocketFd = accept(_fd, (struct sockaddr*)&clientInfos, &clientInfoSize);
	if (newClientSocketFd < 0) {
		Log::prod("error", "accept(): " + utils::str(strerror(errno)));
		return -1;
	}
	Log::dev("setup", "Socket accept() success.");
	return (newClientSocketFd);
}

HostPortPair const&	Socket::getListenDirective() const
{
    return (_listenDirective);
}
