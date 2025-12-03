#include "cgi/CgiContext.hpp"

CgiContext::CgiContext(int cfd, CgiData const& d, int inReadFd, int inWriteFd, int outReadFd, int outWriteFd, int errReadFd, int errWriteFd)
: _pid(-1)
, _clientFd(cfd)
, _cgiData(d)
, _request(d.getRequest())
, _errorPages(d.getErrorPages())
, _inReadFd(inReadFd)
, _inWriteFd(inWriteFd)
, _outReadFd(outReadFd)
, _outWriteFd(outWriteFd)
, _errReadFd(errReadFd)
, _errWriteFd(errWriteFd)
, _output()
, _error()
, _startTime(time(NULL))
, _inputBytesSent(0)
, _headersReceived(false)
, _headersSent(false)
{}

CgiContext::~CgiContext()
{
	closeAllFds();
    Log::dev("debug", "CgiContext destroyed for PID " + utils::str(_pid));
}

void	CgiContext::setPid(int pid)
{
	_pid = pid;
}

void	CgiContext::appendOutput(const char* data, size_t len)
{
	_output.append(data, len);
}

void	CgiContext::appendError(const char* data, size_t len)
{
	_error.append(data, len);
}

void	CgiContext::setStartTime()
{
	_startTime = time(NULL);
}

void	CgiContext::addInputBytesSent(size_t bytes)
{
	_inputBytesSent += bytes;
}

void CgiContext::closeInReadFd()
{
	_safeCloseFd(_inReadFd);
}

void	CgiContext::closeInWriteFd()
{
	_safeCloseFd(_inWriteFd);
}

void	CgiContext::closeOutReadFd()
{
	_safeCloseFd(_outReadFd);
}

void	CgiContext::closeOutWriteFd()
{
	_safeCloseFd(_outWriteFd);
}

void	CgiContext::closeErrReadFd()
{
	_safeCloseFd(_errReadFd);
}

void	CgiContext::closeErrWriteFd()
{
	_safeCloseFd(_errWriteFd);
}

void	CgiContext::closeAllFds()
{
	closeInReadFd();
	closeInWriteFd();
	closeOutReadFd();
	closeOutWriteFd();
	closeErrReadFd();
	closeErrWriteFd();
}

pid_t	CgiContext::getPid() const
{
	return _pid;
}

int	CgiContext::getClientFd() const
{
	return _clientFd;
}

CgiData const&	CgiContext::getCgiData() const
{
	return _cgiData;
}

Request const&	CgiContext::getRequest() const
{
	return _request;
}

ErrorPages const&	CgiContext::getErrorPages() const
{
	return _errorPages;
}

int	CgiContext::getInReadFd() const
{
	return _inReadFd;
}

int	CgiContext::getInWriteFd() const
{
	return _inWriteFd;
}

int	CgiContext::getOutReadFd() const
{
	return _outReadFd;
}

int	CgiContext::getOutWriteFd() const
{
	return _outWriteFd;
}

int	CgiContext::getErrReadFd() const
{
	return _errReadFd;
}

int	CgiContext::getErrWriteFd() const
{
	return _errWriteFd;
}

std::string	const&	CgiContext::getOutput() const
{
	return _output;
}

std::string const&	CgiContext::getError() const
{
	return _error;
}

time_t const&	CgiContext::getStartTime() const
{
	return _startTime;
}

size_t	CgiContext::getInputBytesSent() const
{
	return _inputBytesSent;
}

bool CgiContext::headersReceived() const
{
	return _headersReceived;
}

void CgiContext::setHeadersReceived(bool val)
{
	_headersReceived = val;
}

bool CgiContext::headersSent() const
{
	return _headersSent;
}

void CgiContext::setHeadersSent(bool val)
{
	_headersSent = val;
}

// STATIC UTILS

void	CgiContext::_safeCloseFd(int& fd)
{
	if (fd != -1) {
		close(fd);
		fd = -1;
	}
}

// PRINT

std::ostream&	operator<<(std::ostream& os, CgiContext const& rhs)
{
	os << "- pid: " << rhs.getPid() << "\n";
	os << "- clientFd " << rhs.getClientFd() << "\n";
	os << "- cgiData: [too long to display]\n";
	os << "- request: [too long to display]\n";
	os << "- errorPages:\n";
	for (ErrorPages::const_iterator it = rhs.getErrorPages().begin(); it != rhs.getErrorPages().end(); ++it)
		os << "  " << it->first << " -> " << it->second << "\n";
	os << "- inReadFd: " << utils::str(rhs.getInReadFd()) << "\n";
	os << "- inWriteFd: " << utils::str(rhs.getInWriteFd()) << "\n";
	os << "- outReadFd: " << utils::str(rhs.getOutReadFd()) << "\n";
	os << "- outWriteFd: " << utils::str(rhs.getOutWriteFd()) << "\n";
	os << "- errReadFd: " << utils::str(rhs.getErrReadFd()) << "\n";
	os << "- errWriteFd: " << utils::str(rhs.getErrWriteFd()) << "\n";
	os << "- start time: " << rhs.getStartTime() << "\n";
	os << "- output: " << PrintableString(rhs.getOutput()) << "\n";
	os << "- error: " << PrintableString(rhs.getError()) << "\n";
	return os;
}
