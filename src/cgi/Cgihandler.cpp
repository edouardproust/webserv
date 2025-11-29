#include "cgi/CgiHandler.hpp"

size_t const	CgiHandler::_TIMEOUT_SECONDS = 5;

CgiHandler::CgiHandler()
: _network(NULL)
{}

CgiHandler::~CgiHandler()
{
	killAndCleanupAll();
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

void	CgiHandler::handleError(CgiContext* ctx, std::string const& errorSlug, std::string const& errorMsg)
{
	Log::prod("error", errorMsg);
	// Send error to client
	Response errorResp = StaticHandler::error(errorSlug, ctx->getRequest(), ctx->getErrorPages());
	errorResp.setHeader("Connection", "close"); // force close
	if (_network) {
		_network->onCgiResponseReady(ctx->getClientFd(), errorResp);
	}
	_killAndCleanupProcess(ctx->getPid());
}

void	CgiHandler::_cleanupProcessRessources(pid_t pid)
{
	if (pid == -1)
		return;
	std::map<pid_t, CgiContext*>::iterator it = _contextsByPid.find(pid);
	if (it == _contextsByPid.end())
		return;
	CgiContext* ctx = it->second;

	// Cleanup ressources (pipes, context)
	_contextsByPid.erase(it);
	if (!ctx)
		return;
	int stdoutPipe = ctx->getStdoutPipe();
	if (stdoutPipe != -1) {
		_contextsByPipeFd.erase(stdoutPipe);
		if (_network)
			_network->epollControl(stdoutPipe, EPOLL_CTL_DEL, 0, "CGI cleanup stdout");
		close(stdoutPipe);
	}
	int	stderrPipe = ctx->getStderrPipe();
	if (stderrPipe != -1) {
		_contextsByPipeFd.erase(stderrPipe);
		if (_network)
			_network->epollControl(stderrPipe, EPOLL_CTL_DEL, 0, "CGI cleanup stderr");
		close(stderrPipe);
	}
	delete ctx;
}

/**
 * Kill and clean up one CGI process.
 *
 * Kill the process if running, close pipes and remove them from epoll surveillance,
 * and delete related CgiContext (owned by CgiHandler).
 * The killed process will be reaped on the next checkCompletion() call.
 */
void CgiHandler::_killAndCleanupProcess(pid_t pid)
{
	// Kill process
	int status;
	pid_t result = waitpid(pid, &status, WNOHANG); // check process state
	if (result == 0) { // process still alive
		kill(pid, SIGKILL); // kill process
		_zombiesToReap.insert(pid);  // zombie will be reaped on next checkCompletion()
	}
	// Clean up
	_cleanupProcessRessources(pid);
	Log::dev("event", "CGI process killed and cleaned up (pid " + Log::hl(pid) + ").");
}

/**
 * Kill and clean all running CGI processes and delete the current CGI Handler.
 * Used to flush all memory related to CGI processes when a SIGKILL happens.
 */
void	CgiHandler::fullCleanup()
{
	std::vector<pid_t> allPids;
	for (std::map<pid_t, CgiContext*>::iterator it = _contextsByPid.begin(); it != _contextsByPid.end(); ++it)
		allPids.push_back(it->first);
	for (size_t i = 0; i < allPids.size(); ++i)
		_killAndCleanupProcess(allPids[i]);
	Log::dev("close", "All CGI processes killed and cleaned up.");
	_reapZombies();
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
	time_t now = time(NULL);

	// Make a copy to prevent problesm during iteration
	std::vector<pid_t> pidsToCheck;
	for (std::map<pid_t, CgiContext*>::iterator it = _contextsByPid.begin();
		it != _contextsByPid.end(); ++it) {
		pidsToCheck.push_back(it->first);
	}

	// Check and handle completed CGI processes
	for (size_t i = 0; i < pidsToCheck.size(); ++i) {
		pid_t pid = pidsToCheck[i];
		if (_contextsByPid.find(pid) == _contextsByPid.end())
			continue;
		CgiContext* ctx = _contextsByPid[pid];
		int status = 0;
		pid_t result = waitpid(pid, &status, WNOHANG); // Clean zombie processes (non-blocking)

		if (result == pid) { // Process stopped (reaped by witpid)
			_finalizeCgiResponse(ctx, status);
			_cleanupProcessRessources(pid);
		} else if (result == 0) { // Process still running
			size_t elapsedTime = now - ctx->getStartTime();
			if (elapsedTime > _TIMEOUT_SECONDS) // check timeout
				handleError(ctx, "timeout", "Timeout after " + utils::str(elapsedTime) + " seconds.");
		} else if (result == -1) { // Process already reaped
			Log::dev("warning", "waitpid returned -1 for pid " + Log::hl(pid) + ".");
			_killAndCleanupProcess(pid);
		}
	}
	_reapZombies();
}

void	CgiHandler::_reapZombies()
{
	std::set<pid_t> stillZombies;

	for (std::set<pid_t>::iterator it = _zombiesToReap.begin(); it != _zombiesToReap.end(); ++it) {
		int status;
		pid_t result = waitpid(*it, &status, WNOHANG);
		if (result == *it)
			Log::dev("event", "Zombie reaped (pid " + Log::hl(*it) + ").");
		else // race condition (kill not finalized: process not reaped yet)
			stillZombies.insert(*it);
	}
	_zombiesToReap.swap(stillZombies);
}

bool	CgiHandler::isCgiPipe(int fd) const
{
	return _contextsByPipeFd.find(fd) != _contextsByPipeFd.end();
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

// TODO
void	CgiHandler::_addPipeToEpoll(int fd, std::string const& context)
{
	(void)fd, (void)context;
	Log::dev("debug", "CgiHandler::_addPipeToEpoll"); // DEBUG
}
