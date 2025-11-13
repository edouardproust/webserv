#include "network/Socket.hpp"

Socket::Socket(const HostPortPair &listen_pair) : _listenOn(listen_pair)
{
	Log::dev("setup", "Setting up socket on " + Log::hl(_listenOn) + ".");

	loadAddressInfo();
	createSocket();

	Log::dev("setup", "Socket created on " + Log::hl(_listenOn) + ".");
}

Socket::~Socket()
{
	close(_sock);
	Log::prod("ok", "Socket " + Log::hl(_listenOn) + " closed.");
}

void Socket::setAddrStruct(void)
{
	std::memset(&_hints, 0, sizeof(_hints));
	_hints.ai_family = AF_UNSPEC;	  // IPv4 only
	_hints.ai_socktype = SOCK_STREAM; // TCP or UDP
	_hints.ai_flags = AI_PASSIVE;	  // allows bind
}

void Socket::loadAddressInfo()
{
	Log::dev("setup", "Loading address info...");
	int status;
	setAddrStruct();

	// Convert size_t (port) to C-string
	std::string portStr = utils::str(_listenOn.getPort());

	// Get host. If it's "0.0.0.0" or "*", we use NULL for getaddrinfo
	std::string hostStr = _listenOn.getHost();
	const char* host = (hostStr == "*" || hostStr == "0.0.0.0") ? NULL : hostStr.c_str();

	// Use the values from _listenOn
	status = getaddrinfo(host, portStr.c_str(), &_hints, &_servinfo);

	if (status) {
		Log::prod("status", "Address info status: " + utils::str(gai_strerror(status)));
		throw GetAddrInfoException();
	}
}

void Socket::createSocket()
{
	_sock = ::socket(_servinfo->ai_family, _servinfo->ai_socktype, _servinfo->ai_protocol);

	Log::dev("setup", "Creating Socket...");
	if (_sock < 0) {
		::freeaddrinfo(_servinfo);
		throw SocketException();
	}

	int yes = 1;

	Log::dev("setup", "Configuring Socket...");
	if (::setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
		::freeaddrinfo(_servinfo);
		throw SetSockOptException();
	}

	Log::dev("setup", "Setting Socket to non-blocking mode...");
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

	Log::dev("setup", "Binding socket on " + Log::hl(_listenOn) + "...");

	status = ::bind(_sock, _servinfo->ai_addr, _servinfo->ai_addrlen);

	if (status < 0) {
		freeaddrinfo(_servinfo);
		Log::prod("status",  "Bind status: " + utils::str(strerror(errno)));
		throw BindException();
	}

	Log::dev("setup", "Freeing Address Info memory...");
	freeaddrinfo(_servinfo);
}

void Socket::listen()
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

int Socket::accept()
{
	struct sockaddr_storage their_addr;
	socklen_t addr_size;
	int new_socket;

	Log::dev("event", "accept() requested.");
	addr_size = sizeof their_addr;
	new_socket = ::accept(_sock, (struct sockaddr *)&their_addr, &addr_size);
	if (new_socket < 0) {
		Log::prod("error", "accept(): " + utils::str(strerror(errno)));
		throw AcceptException();
	}
	Log::dev("ok", "accept() successfull");
	return (new_socket);
}

const HostPortPair& Socket::getHostPortPair() const
{
    return (_listenOn);
}

const char *Socket::GetAddrInfoException::what() const throw()
{
	return ("Can't get Address's info.");
}

const char *Socket::SocketException::what() const throw()
{
	return ("Can't create Socket.");
}

const char *Socket::SetSockOptException::what() const throw()
{
	return ("Can't configure Socket.");
}

const char *Socket::FcntlException::what() const throw()
{
	return ("Can't set Socket to non blocking mode.");
}

const char *Socket::BindException::what() const throw()
{
	return ("Can't bind socket with IP and port.");
}

const char *Socket::ListenException::what() const throw()
{
	return ("Can't put Socket in Listen Mode.");
}

const char *Socket::AcceptException::what() const throw()
{
	return ("Accept rejected.");
}

const char *Socket::ConnectException::what() const throw()
{
	return ("Connect rejected.");
}