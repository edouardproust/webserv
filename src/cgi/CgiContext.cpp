#include "cgi/CgiContext.hpp"

CgiContext::CgiContext(pid_t p, int cfd, int inWriteFd, int outReadFd, int errReadFd, CgiData const& d)
: _pid(p)
, _clientFd(cfd)
, _stdinWriteFd(inWriteFd)
, _stdoutReadFd(outReadFd)
, _stderrReadFd(errReadFd)
, _request(d.getRequest())
, _errorPages(d.getErrorPages())
, _startTime(time(NULL))
, _output()
, _error()
{}

CgiContext::~CgiContext()
{}

void	CgiContext::setStartTime()
{
	_startTime = time(NULL);
}

void	CgiContext::appendOutput(const char* data, size_t len)
{
	_output.append(data, len);
}

void	CgiContext::appendError(const char* data, size_t len)
{
	_error.append(data, len);
}

void	CgiContext::closeStdinWriteFd()
{
	if (_stdinWriteFd != -1) {
		close(_stdinWriteFd);
		_stdinWriteFd = -1;
	}
}

void	CgiContext::closeStdoutReadFd()
{
	if (_stdoutReadFd != -1) {
		close(_stdoutReadFd);
		_stdoutReadFd = -1;
	}
}

void	CgiContext::closeStderrReadFd()
{
	if (_stderrReadFd != -1) {
		close(_stderrReadFd);
		_stderrReadFd = -1;
	}
}

pid_t	CgiContext::getPid() const
{
	return _pid;
}

int	CgiContext::getClientFd() const
{
	return _clientFd;
}

int	CgiContext::getStdinWriteFd() const
{
	return _stdinWriteFd;
}

int	CgiContext::getStdoutReadFd() const
{
	return _stdoutReadFd;
}

int	CgiContext::getStderrReadFd() const
{
	return _stderrReadFd;
}

Request const&	CgiContext::getRequest() const
{
	return _request;
}

ErrorPages const&	CgiContext::getErrorPages() const
{
	return _errorPages;
}

time_t const&	CgiContext::getStartTime() const
{
	return _startTime;
}

std::string	const&	CgiContext::getOutput() const
{
	return _output;
}

std::string const&	CgiContext::getError() const
{
	return _error;
}

std::ostream&	operator<<(std::ostream& os, CgiContext const& rhs)
{
	os << "CgiContext:\n";
	os << "- pid: " << rhs.getPid() << "\n";
	os << "- clientFd " << rhs.getClientFd() << "):\n";
	os << "- stdinWriteFd: " << utils::str(rhs.getStdinWriteFd()) << "\n";
	os << "- stdoutReadFd: " << utils::str(rhs.getStdoutReadFd()) << "\n";
	os << "- stderrReadFd: " << utils::str(rhs.getStderrReadFd()) << "\n";
	os << "- start time: " << rhs.getStartTime() << "\n";
	os << "- output: " << PrintableString(rhs.getOutput()) << "\n";
	os << "- error: " << PrintableString(rhs.getError()) << "\n";
	return os;
}
