#include "cgi/SafePipe.hpp"

/**
 * throws exception in case of pipe() or fnctl() failure.
 */
SafePipe::SafePipe(std::string const& description)
: _readFd(-1)
, _writeFd(-1)
, _description(description)
{
	_create();
	_setNonBlocking();
}

SafePipe::~SafePipe()
{}

void	SafePipe::_safeClose(int& fd, std::string const& end)
{
	if (fd == -1)
		return;
	if (close(fd) == -1)
		Log::dev("warning", "close() failed for " + _description + " " + end + " (fd " + utils::str(fd) + "): " + strerror(errno));
	else
		Log::dev("close", "Closed " + _description + " " + end + " (fd " + utils::str(fd) + ")");
	fd = -1;
}

/**
 * Create the pipe. Returns true on success, false on failure.
 * Throws an exception if pipe() fails.
 */
void	SafePipe::_create()
{
	int fds[2];
	if (pipe(fds) == -1) {
		throw std::runtime_error("pipe() failed for " + _description + ": " + strerror(errno));
	}
	_readFd = fds[0];
	_writeFd = fds[1];
	Log::dev("setup", "Created pipe " + utils::str(*this) + ".");
}

/**
 * Set both read and write ends to non-blocking mode
 * Returns true if both succeeded, false otherwise.
 * Throws an exception if fcntl() fails.
 */
void	SafePipe::_setNonBlocking()
{
	bool readOk = true, writeOk = true;
	if (_readFd != -1) {
		int flags = fcntl(_readFd, F_GETFL, 0);
		readOk = (flags != -1) && (fcntl(_readFd, F_SETFL, flags | O_NONBLOCK) != -1);
	}
	if (_writeFd != -1) {
		int flags = fcntl(_writeFd, F_GETFL, 0);
		writeOk = (flags != -1) && (fcntl(_writeFd, F_SETFL, flags | O_NONBLOCK) != -1);
	}
	if (!readOk || !writeOk) {
		throw std::runtime_error("Failed to set non-blocking for " + _description + " (read:" + (readOk ? "OK" : "FAIL") + ", write:" + (writeOk ? "OK" : "FAIL") + ")");
	}
	Log::dev("setup", "Pipe " + _description + " set to non-blocking mode.");
}

/**
 * Close only the read end
 */
void	SafePipe::closeRead()
{
	_safeClose(_readFd, "read end");
}

/**
 * Close only the write end
 */
void	SafePipe::closeWrite()
{
	_safeClose(_writeFd, "write end");
}

int	SafePipe::readFd() const
{
	return _readFd;
}

int	SafePipe::writeFd() const
{
	return _writeFd;
}

const std::string&	SafePipe::getDescription() const
{
	return _description;
}

std::ostream&	operator<<(std::ostream& os, SafePipe const& rhs)
{
	os << PrintableString(rhs.getDescription())
		<< " (" << "readFd=" << rhs.readFd() << ", writeFd=" << rhs.writeFd() << ")";
	return os;
}

