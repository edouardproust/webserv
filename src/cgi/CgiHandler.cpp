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

	try {
		// create pipes
		SafePipe stdinPipe("CGI stdin");
		SafePipe stdoutPipe("CGI stdout");
		SafePipe stderrPipe("CGI stderr");
		pid_t pid = fork();
		if (pid < 0)
			throw std::runtime_error("Failed to fork CGI process");

		if (pid == 0) // Child
			_setupChildProcess(stdinPipe, stdoutPipe, stderrPipe, d);
		else // Parent
			_setupParentProcess(stdinPipe, stdoutPipe, stderrPipe, clientFd, pid, d);
	} catch (std::exception& e) {
        Log::prod("error", "CGI setup for client fd " + Log::hl(clientFd) + ": " + utils::str(e.what())); // TODO log in StaticHandler::error instead?
		errorFromClientFd(clientFd, "internal_error", d.getRequest(), d.getErrorPages(), "CGI setup failed");
    }
}

void CgiHandler::_setupChildProcess(SafePipe& stdinPipe, SafePipe& stdoutPipe, SafePipe& stderrPipe, CgiData const& data)
{
	// Close unused pipe ends
	stdinPipe.closeWrite(); // child does not write on stdin
    stdoutPipe.closeRead(); // child does not read on stdout
    stderrPipe.closeRead(); // child does not read on stderr

	// Redirect pipes
	if (dup2(stdinPipe.readFd(), STDIN_FILENO) == -1)
		_exit(1);
	if (dup2(stdoutPipe.writeFd(), STDOUT_FILENO) == -1)
		_exit(1);
	if (dup2(stderrPipe.writeFd(), STDERR_FILENO) == -1)
		_exit(1);

	// Close original pipes (now redirected)
	stdinPipe.closeRead();
	stdoutPipe.closeWrite();
	stderrPipe.closeWrite();

	// Launch executable
	execve(data.getExecutor().c_str(), data.getArgv().data(), data.getEnvp().data());
	_exit(1); // execve failed if this line is reached
}

//TODO
void CgiHandler::_setupParentProcess(SafePipe& stdinPipe, SafePipe& stdoutPipe, SafePipe& stderrPipe, int clientFd, pid_t pid, CgiData const& data)
{
	Log::dev("setup", "Parent process CGI setup (pid " + Log::hl(pid) + ", client fd " + Log::hl(clientFd) + ").");

	// Close unused pipe ends
	stdinPipe.closeRead(); // Parent doesn't read from stdin
	stdoutPipe.closeWrite(); // Parent doesn't write to stdout
	stderrPipe.closeWrite(); // Parent doesn't write to stderr

	// Create and store CgiContext
	CgiContext* ctx = new CgiContext(pid, clientFd, stdinPipe.writeFd(), stdoutPipe.readFd(), stderrPipe.readFd(), data);
	_contextsByPid[pid] = ctx;
	_contextsByPipeFd[stdoutPipe.readFd()] = ctx;
	_contextsByPipeFd[stderrPipe.readFd()] = ctx;
	Log::dev("setup", "Created cgiContext for pid " + Log::hl(pid) + ".");
	Log::dev("debug", utils::str(*ctx));

	try {
		_network->epollControl(stdoutPipe.readFd(), EPOLL_CTL_ADD, EPOLLIN, "CGI stdout");
		_network->epollControl(stderrPipe.readFd(), EPOLL_CTL_ADD, EPOLLIN, "CGI stderr");
		Log::dev("setup", "CGI pipes added to epoll (stdout fd " + Log::hl(stdoutPipe.readFd()) + ", stderr fd " + Log::hl(stderrPipe.readFd()) + ").");
	} catch (std::exception& e) {
		_killAndCleanupProcess(pid);
		throw;
	}
}

// TODO
void	CgiHandler::readAndAccumulateCgiOutput(int pipeFd)
{
	Log::dev("todo", "CgiHandler::handlePipeRead\n"
		"- pipeFd: " + utils::str(pipeFd)
	); // DEBUG

	std::map<int, CgiContext*>::iterator it = _contextsByPipeFd.find(pipeFd);
	if (it == _contextsByPipeFd.end()) return;

	CgiContext* ctx = it->second;
	char buffer[4096];
	ssize_t bytes = read(pipeFd, buffer, sizeof(buffer));

	// TODO this is blocking: do a reading by chunks
	if (bytes > 0) {
		if (pipeFd == ctx->getStdoutReadFd()) {
			ctx->appendOutput(buffer, bytes);
		}
		// TODO same thing for stderr
		// No POLLOUT here yet: CGI is still running...
	}
	Log::dev("todo", "CGI output: " + ctx->getOutput()); // DEBUG
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

		if (result == pid) { // Process stopped (reaped by waitpid)
			_finalizeResponse(ctx, status);
			_cleanupProcessRessources(pid);
		} else if (result == 0) { // Process still running
			size_t elapsedTime = now - ctx->getStartTime();
			if (elapsedTime > _TIMEOUT_SECONDS) // check timeout
				_errorFromContext(ctx, "timeout", "Timeout after " + utils::str(elapsedTime) + " seconds.");
		} else if (result == -1) { // Process already reaped
			Log::dev("warning", "waitpid returned -1 for pid " + Log::hl(pid) + ".");
			_killAndCleanupProcess(pid);
		}
	}
	_reapZombies();
}

bool	CgiHandler::isCgiPipe(int fd) const
{
	return _contextsByPipeFd.find(fd) != _contextsByPipeFd.end();
}

// TODO
void	CgiHandler::_finalizeResponse(CgiContext* ctx, int exitStatus)
{

	(void)exitStatus;
	Log::dev("event", "CGI process completed");
	// DEBUG: send placeholder response
	Response resp = StaticHandler::cgiPlaceholder();
	_network->prepareResponseSend(ctx->getClientFd(), resp);
	_network->epollControl(ctx->getClientFd(), EPOLL_CTL_MOD, EPOLLOUT, "CGI response ready"); // TODO catch
	Log::dev("event", "Switched client to EPOLLOUT for CGI response");
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
	_killAndCleanupProcess(ctx->getPid());
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
