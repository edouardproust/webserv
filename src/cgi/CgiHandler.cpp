#include "cgi/CgiHandler.hpp"

CgiHandler::CgiHandler()
{}

CgiHandler::~CgiHandler()
{}

void	CgiHandler::init(int epollFd)
{
	_epollFd = epollFd;
}

void	CgiHandler::launchAsync(int clientFd, Response const& cgiResponse,
	std::map<int, std::string>& pendingResponses, std::map<int, size_t>& responseSendPos,
	std::map<int, bool>& shouldCloseAfterResponse)
{
	(void)clientFd, (void)cgiResponse, (void)pendingResponses, (void)responseSendPos, (void)shouldCloseAfterResponse;
}

void	CgiHandler::handlePipeRead(int pipeFd)
{
	(void)pipeFd;
}

void	CgiHandler::checkCompletion()
{}

void	CgiHandler::cleanupAll()
{}

bool	CgiHandler::isCgiPipe(int fd) const
{
	return _contextsByPipe.find(fd) != _contextsByPipe.end();
}
