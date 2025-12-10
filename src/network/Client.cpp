#include "network/Client.hpp"
#include "network/Network.hpp"

size_t const	Client::_CLOSE_TIMEOUT_SECONDS = 10;
size_t const	Client::_KEEPALIVE_TIMEOUT_SECONDS = 60;

Client::Client(int fd, Socket* socket, Network* network)
: _fd(fd)
, _socket(socket)
, _network(network)
, _lastActivity(time(NULL))
, _pendingRequest() //!\ creates a new emmpty Request object
, _pendingResponse()
, _responseSendPos(0)
, _shouldCloseAfterResponse(true) //!\ close by default
{}

Client::~Client()
{}

void Client::closeSocket()
{
	if (_fd != -1) {
		close(_fd);
		_fd = -1;
	}
}

/**
 * Sends the next part of the pending HTTP response.
 * Continues until everything is sent or the next EPOLLOUT event.
 * Called when EPOLLOUT indicates the client is ready for more data.
 *
 * @note This method is protected by epoll (EPOLLOUT) and is non-blocking (sending by chunks).
 * This is the only place we write to a client socket.
 * No errno-based decision-making occurs after send().
 */
bool	Client::continueResponseSend()
{
	if (_pendingResponse.empty()) {
		Log::dev("warning", "No pending response for fd " + utils::str(_fd));
		return true; // no client disconnection
	}

	// Send remaining part of the response
	size_t remaining = _pendingResponse.length() - _responseSendPos;
	ssize_t bytesSent = send(_fd, _pendingResponse.data() + _responseSendPos, remaining, 0);

	if (bytesSent < 0) {
		// Sending error -> Disconnect client
		Log::prod("error", "Send failed for fd " + Log::hl(_fd) + ".");
		return false; // client disconnection
	}
	_responseSendPos += bytesSent;

	// Check if send if complete
	if (_responseSendPos >= _pendingResponse.size()) {
		Log::prod("ok", "Response sent to client (fd " + Log::hl(_fd) + ").");
		// Cleanup
		_pendingResponse.clear();
		_responseSendPos = 0;
		// Handle connection
		if (_shouldCloseAfterResponse) {
			return false; // client disconnection
		} else {
			_resetForNextRequest(); // keep-alive
			return true; // no client disconnection
		}
	}
	Log::dev("event", "Sent " + Log::hl(_responseSendPos) + "/" + utils::str(_pendingResponse.size()) + " bytes to client " + Log::hl(_fd) + ".");
	return true; // Stay in EPOLLOUT mode to continue sending
}

void	Client::prepareResponseSend(Response const& response)
{
	std::string rawResponse = response.stringify();
	_pendingResponse = rawResponse;
	_responseSendPos = 0;
	_shouldCloseAfterResponse = response.isConnectionClose();

	Log::dev("debug", "Response:\n" + utils::str(response));
	Log::dev("cgi", "Queued " + Log::hl(rawResponse.size()) + " bytes to fd " + Log::hl(_fd));
}

/**
 * Resets the client's internal state so it can process another request on a keep-alive connection.
 */
void	Client::_resetForNextRequest()
{
	_pendingRequest = Request(); //!\ Create a new empty Request object
	_pendingResponse.clear();
	_responseSendPos = 0;
	updateActivity();
	// we keep _shouldCloseAfterResponse at the value it was before

	Log::dev("event", "Connection: keep-alive -> Resetting fd " + Log::hl(_fd) + " to 'recv'.");
	_network->epollControl(_fd, EPOLL_CTL_MOD, EPOLLIN, "keep-alive reset"); // throw
}

bool	Client::isInactive(time_t now) const
{
	time_t timeout;
	if (_shouldCloseAfterResponse)
		timeout = (time_t)_CLOSE_TIMEOUT_SECONDS;
	else
		timeout = (time_t)_KEEPALIVE_TIMEOUT_SECONDS;
	if (now - _lastActivity >= timeout) {
		Log::dev("status", "Client (fd " + Log::hl(_fd) + ") is inactive for " + Log::hl(timeout) + " seconds.");
		return true;
	}
	return false;
}

// SETTERS

void	Client::updateActivity()
{
	_lastActivity = time(NULL);
}

// GETTERS

int	Client::getFd() const
{
	return _fd;
}

Socket*	Client::getSocket() const
{
	return _socket;
}

time_t	Client::getLastActivity() const
{
	return _lastActivity;
}

Request const&	Client::getRequest() const
{
	return _pendingRequest;
}

Request&	Client::getRequest()
{
	return _pendingRequest;
}

std::string const&	Client::getResponse() const
{
	return _pendingResponse;
}

size_t const&	Client::getResponseSendPos() const
{
	return _responseSendPos;
}

bool	Client::shouldCloseAfterResponse() const
{
	return _shouldCloseAfterResponse;
}


// PRINT

std::ostream& operator<<(std::ostream& os, Client const& rhs)
{
	os << " - FD: " << rhs.getFd() << "\n";
	os << " - Socket FD: " << utils::str(rhs.getSocket() ? rhs.getSocket()->getFd() : -1) << "\n";

	if (rhs.getSocket())
		os << "- Listen directive: " << rhs.getSocket()->getListenDirective() << "\n";

	os << " - Last activity: " << rhs.getLastActivity() << " (epoch time)\n";
	os << " - Pending request size: " << utils::str(rhs.getRequest().getRawRequest().size()) << " bytes\n";
	os << " - Pending response size: " << rhs.getResponse().size() << " bytes\n";
	os << " - Response send position: " << rhs.getResponseSendPos() << "\n";
	os << " - Should close after response: " << (rhs.shouldCloseAfterResponse() ? "yes" : "no") << "\n";

	return os;
}
