#include "cgi/CgiContext.hpp"

CgiContext::CgiContext(pid_t p, int cfd, int out, int err, bool closeConn, Request const* req)
: _pid(p)
, _clientFd(cfd)
, _stdoutPipe(out)
, _stderrPipe(err)
, _startTime(time(NULL))
, _connectionClose(closeConn)
, _originalRequest(req)
{}

CgiContext::~CgiContext()
{}

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