#include "cgi/CgiHandler.hpp"

CgiHandler::CgiHandler()
: _network(NULL)
{}

CgiHandler::~CgiHandler()
{
	cleanupAll();
}

// TODO
void	CgiHandler::init(Network* network)
{
	Log::dev("debug", "CgiHandler::init"); // DEBUG
	_network = network;
}

// TODO
void	CgiHandler::launchAsync(int clientFd, Response const& cgiResponse)
{
	(void)clientFd, (void)cgiResponse;
	Log::dev("debug", "CgiHandler::launchAsync"); // DEBUG
}

// TODO
void	CgiHandler::handlePipeRead(int pipeFd)
{
	(void)pipeFd;
	Log::dev("debug", "CgiHandler::handlePipeRead"); // DEBUG
}

void	CgiHandler::handlePipeError(int pipeFd)
{
	if (_contextsByPipeFd.find(pipeFd) == _contextsByPipeFd.end())
		return;

	CgiContext* ctx = _contextsByPipeFd[pipeFd];
	Log::prod("error", "CGI pipe error for client " + Log::hl(ctx->getClientFd()));

	// Nettoyer ce pipe spécifique
	_cleanup(ctx);

	// Envoyer erreur au client
	if (_network) {
		_network->handleCgiError(ctx->clientFd, "internal_server_error");
	}
}

// TODO
/**
 * Check on each epoll_wait loop if any CGI Responses are ready to be finialized.
 * If so, finilize them.
 *
 * @note This method is non-blocking thanks to `WNOHANG`.
 */
void	CgiHandler::checkCompletion()
{
	Log::dev("debug", "CgiHandler::checkCompletion"); // DEBUG
}

// TODO
void	CgiHandler::_launchProcess(int clientFd, Response const& response)
{
	(void)clientFd, (void) response;
	Log::dev("debug", "CgiHandler::_launchProcess"); // DEBUG
}

// TODO
void	CgiHandler::_finalizeResponse(CgiContext* ctx, int exitStatus)
{
	(void)ctx, (void)exitStatus;
	Log::dev("debug", "CgiHandler::_finalizeResponse"); // DEBUG
}

/**
 * Clean up one running CGI process.
 *
 * Close pipes and remove them from epoll surveillance.
 * Delete related CgiContext (owned by CgiHandler).
 */
void CgiHandler::_cleanup(CgiContext* ctx)
{
	if (!ctx) return;

	int stdoutPipe = ctx->getStdoutPipe();
	if (stdoutPipe != -1) {
		if (_network)
			_network->epollControl(stdoutPipe, EPOLL_CTL_DEL, 0, "CGI cleanup stdout");
		close(stdoutPipe);
		_contextsByPipeFd.erase(stdoutPipe);
	}

	int	stderrPipe = ctx->getStderrPipe();
	if (stderrPipe != -1) {
		if (_network)
			_network->epollControl(stderrPipe, EPOLL_CTL_DEL, 0, "CGI cleanup stderr");
		close(stderrPipe);
		_contextsByPipeFd.erase(stderrPipe);
	}

	_contextsByPid.erase(ctx->getPid());
	delete ctx;

	Log::dev("event", "CGI context cleaned up");
}

// TODO
void	CgiHandler::_addPipeToEpoll(int fd, std::string const& context)
{
	(void)fd, (void)context;
	Log::dev("debug", "CgiHandler::_addPipeToEpoll"); // DEBUG
}

/**
 * Kill all running CGI processes.
 */
void	CgiHandler::cleanupAll()
{
	for (std::map<pid_t, CgiContext*>::iterator it = _contextsByPid.begin();
		it != _contextsByPid.end(); ++it)
	{
		CgiContext* ctx = it->second;
		pid_t pid = ctx->getPid();
		if (ctx && pid != -1) {
			kill(pid, SIGKILL);
			waitpid(pid, NULL, 0);
			_cleanup(ctx);
		}
	}
	_contextsByPid.clear();
	_contextsByPipeFd.clear();

	Log::dev("close", "All CGI processes cleaned up");
}

bool	CgiHandler::isCgiPipe(int fd) const
{
	return _contextsByPipeFd.find(fd) != _contextsByPipeFd.end();
}
