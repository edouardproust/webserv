#include "cgi/CgiHandler.hpp"
#include "network/Network.hpp"

size_t const	CgiHandler::_TIMEOUT_SECONDS = 5;

CgiHandler::CgiHandler(Network* network)
: _network(network)
{}

CgiHandler::~CgiHandler()
{
	fullCleanup();
}


// MAIN LOGIC

// TODO
void	CgiHandler::launchAsync(int clientFd, Response const& resp)
{
	CgiData const& d = resp.getCgiData();
	if (!CGI_ENABLED) {
		_sendNotImplementedResponse(clientFd, d);
		return;
	}
	Log::dev("todo", "CgiHandler::launchAsync on client fd " + Log::hl(clientFd) + "."); // DEBUG

	try
	{
		// create pipes and set them as non-blocking
		int stdinPipe[2];
		int stdoutPipe[2];
		int stderrPipe[2];
		pipe(stdinPipe); // todo failure
		pipe(stdoutPipe); // todo failure
		pipe(stderrPipe); // todo failure
		fcntl(stdinPipe[0],  F_SETFL, O_NONBLOCK); // todo failure
		fcntl(stdinPipe[1],  F_SETFL, O_NONBLOCK); // todo failure
		fcntl(stdoutPipe[0], F_SETFL, O_NONBLOCK); // todo failure
		fcntl(stdoutPipe[1], F_SETFL, O_NONBLOCK); // todo failure
		fcntl(stderrPipe[0], F_SETFL, O_NONBLOCK); // todo failure
		fcntl(stderrPipe[1], F_SETFL, O_NONBLOCK); // todo failure

		pid_t pid = fork();
		if (pid < 0)
			throw std::runtime_error("Failed to fork CGI process");
		if (pid == 0) // Child
			_setupChildProcess(d, stdinPipe[0], stdinPipe[1], stdoutPipe[0], stdoutPipe[1], stderrPipe[0], stderrPipe[1]);
		else // Parent
			_setupParentProcess(clientFd, pid, d, stdinPipe[0], stdinPipe[1], stdoutPipe[0], stdoutPipe[1], stderrPipe[0], stderrPipe[1]);
	}
	catch (std::exception& e) {
        Log::prod("error", "CGI setup for client fd " + Log::hl(clientFd) + ": " + utils::str(e.what())); // TODO log in StaticHandler::error instead?
		errorFromClientFd(clientFd, "internal_error", d.getRequest(), d.getErrorPages(), "CGI setup failed");
    }
}

void CgiHandler::_setupChildProcess(CgiData const& data, int inReadFd, int inWriteFd, int outReadFd, int outWriteFd, int errReadFd, int errWriteFd)
{
	// Close unused pipe ends
	close(inWriteFd); // child does not write on stdin
	close(outReadFd); // child does not read on stdout
	close(errReadFd); // child does not read on stderr

	// Redirect pipes
	if (dup2(inReadFd, STDIN_FILENO) == -1)
		_exit(1);
	if (dup2(outWriteFd, STDOUT_FILENO) == -1)
		_exit(1);
	if (dup2(errWriteFd, STDERR_FILENO) == -1)
		_exit(1);

	// Close original pipes (now redirected)
	close(inReadFd);
	close(outWriteFd);
	close(errWriteFd);

	// Launch executable
	execve(data.getExecutor().c_str(), data.getArgv().data(), data.getEnvp().data());
	_exit(1); // execve failed if this line is reached
}

//TODO
void CgiHandler::_setupParentProcess(int clientFd, pid_t pid, CgiData const& data, int inReadFd, int inWriteFd, int outReadFd, int outWriteFd, int errReadFd, int errWriteFd)
{
	Log::dev("setup", "Parent process CGI setup (pid " + Log::hl(pid) + ", client fd " + Log::hl(clientFd) + ").");

	// Close unused pipe ends
	close(inReadFd); // Parent doesn't read from stdin
	close(outWriteFd); // Parent doesn't write to stdout
	close(errWriteFd); // Parent doesn't write to stderr

	// Create and store CgiContext
	CgiContext* ctx = new CgiContext(pid, clientFd, inWriteFd, outReadFd, errReadFd, data);
	_contextsByPid[pid] = ctx;
	_contextsByPipeFd[outReadFd] = ctx;
	_contextsByPipeFd[errReadFd] = ctx;
	Log::dev("setup", "Created cgiContext for pid " + Log::hl(pid) + ".");
	Log::dev("debug", utils::str(*ctx));

	_network->epollControl(outReadFd, EPOLL_CTL_ADD, EPOLLIN, "CGI stdout"); // todo failure
	_network->epollControl(errReadFd, EPOLL_CTL_ADD, EPOLLIN, "CGI stderr"); // todo failure
}

// TODO
void	CgiHandler::readAndAccumulateCgiOutput(int pipeFd)
{
	(void)pipeFd;
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

}

bool	CgiHandler::isCgiPipe(int fd) const
{
	return _contextsByPipeFd.find(fd) != _contextsByPipeFd.end();
}

// TODO
void	CgiHandler::_finalizeResponse(CgiContext* ctx, int exitStatus)
{
	(void)ctx;
	(void)exitStatus;
}

std::map<pid_t, CgiContext*> const&	CgiHandler::getContextsByPid() const
{
	return _contextsByPid;
}






























// ERRORS & CLEANUP

/**
 * Does not clean up ressources.
 */
void CgiHandler::errorFromClientFd(int clientFd, std::string const& errorSlug, Request const& request, ErrorPages const& errorPages, std::string const& errorMsg)
{
	Log::prod("error", errorMsg);
	Response errorResp = StaticHandler::error(errorSlug, request, errorPages);
	errorResp.setHeader("Connection", "close"); // force close
	if (_network) {
		_network->prepareResponseSend(clientFd, errorResp);
		_network->epollControl(clientFd, EPOLL_CTL_MOD, EPOLLOUT, "error response after CGI disabled");
	}
}

/**
 * Cleans up ressources
 */
void	CgiHandler::_errorFromContext(CgiContext* ctx, std::string const& errorSlug, std::string const& errorMsg)
{
	errorFromClientFd(ctx->getClientFd(), errorSlug, ctx->getRequest(), ctx->getErrorPages(), errorMsg);
	//_killAndCleanupProcess(ctx->getPid()); // TODO
}

/**
 * Cleans up ressources
 */
void	CgiHandler::errorFromPipeFd(int pipeFd, std::string const& errorSlug, std::string const& errorMsg)
{
	std::map<int, CgiContext*>::iterator it = _contextsByPipeFd.find(pipeFd);
	if (it == _contextsByPipeFd.end())
		return;
	_errorFromContext(it->second, errorSlug, errorMsg);
}

void	CgiHandler::_cleanupProcessRessources(pid_t pid)
{
	if (pid == -1)
		return;
	std::map<pid_t, CgiContext*>::iterator it = _contextsByPid.find(pid);
	if (it == _contextsByPid.end())
		return;
	CgiContext* ctx = it->second;
	_contextsByPid.erase(it);
	if (!ctx) return;
	Log::dev("debug", "CONTEXT BEFORE CLEANUP:\n" + utils::str(*ctx));


	// Cleanup ressources (pipes, context)
	int stdoutReadFd = ctx->getStdoutReadFd();
	int	stderrReadFd = ctx->getStderrReadFd();
	int	stdinWriteFd = ctx->getStdinWriteFd();
	if (stdoutReadFd != -1) {
		_contextsByPipeFd.erase(stdoutReadFd);
		try {
		_network->epollControl(stdoutReadFd, EPOLL_CTL_DEL, 0, "CGI cleanup stdout"); // TODO catch
		} catch (...) {
			Log::dev("warning", "Failed to remove CGI stdout pipe fd " + utils::str(stdoutReadFd) + " from epoll.");
		}
		close(stdoutReadFd);
	}
	if (stderrReadFd != -1) {
		_contextsByPipeFd.erase(stderrReadFd);
		try {
			_network->epollControl(stderrReadFd, EPOLL_CTL_DEL, 0, "CGI cleanup stderr"); // TODO catch
		} catch (...) {
			Log::dev("warning", "Failed to remove CGI stderr pipe fd " + utils::str(stderrReadFd) + " from epoll.");
		}
		close(stderrReadFd);
	}
	if (stdinWriteFd != -1) {
		close(stdinWriteFd);
	}
	delete ctx;
	ctx = NULL;
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


// BUILTIN RESPONSES

void CgiHandler::_sendNotImplementedResponse(int clientFd, CgiData const& data)
{
	Response resp = StaticHandler::error("not_implemented", data.getRequest(), data.getErrorPages());
	_network->prepareResponseSend(clientFd, resp);
	_network->epollControl(clientFd, EPOLL_CTL_MOD, EPOLLOUT, "CGI 501 response");
}

void	CgiHandler::_sendPlaceholderResponse(int clientFd)
{
	Response resp = StaticHandler::cgiPlaceholder();
	_network->prepareResponseSend(clientFd, resp);
	_network->epollControl(clientFd, EPOLL_CTL_MOD, EPOLLOUT, "CGI placeholder response");
}
