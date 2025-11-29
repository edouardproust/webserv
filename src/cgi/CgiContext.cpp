#include "cgi/CgiContext.hpp"

CgiContext::CgiContext(pid_t p, int cfd, int out, int err, Request const& req, ErrorPages const& ep)
: _pid(p)
, _clientFd(cfd)
, _stdoutPipe(out)
, _stderrPipe(err)
, _request(req)
, _errorPages(ep)
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

pid_t	CgiContext::getPid() const
{
	return _pid;
}

int	CgiContext::getClientFd() const
{
	return _clientFd;
}

int	CgiContext::getStdoutPipe() const
{
	return _stdoutPipe;
}

int	CgiContext::getStderrPipe() const
{
	return _stderrPipe;
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
