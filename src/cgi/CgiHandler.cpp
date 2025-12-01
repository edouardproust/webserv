#include "cgi/CgiHandler.hpp"
#include "network/Network.hpp"

size_t const	CgiHandler::_TIMEOUT_SECONDS = 5;

CgiHandler::CgiHandler()
: _network(NULL)
{}

CgiHandler::~CgiHandler()
{
	fullCleanup();
}

// TODO
void	CgiHandler::init(Network* network)
{
	Log::dev("todo", "CgiHandler::init"); // DEBUG
	_network = network;
}

// TODO
void	CgiHandler::launchAsync(int clientFd, Response const& resp)
{
	Log::dev("todo", "CgiHandler::launchAsync\n"
		"clientFd: " + utils::str(clientFd) + "\nResponse: " + utils::str(resp)); // DEBUG
	/*
	Log::dev("event", "Launching async CGI for client " + Log::hl(clientFd));
	CgiData const& data = resp.getCgiData();
	Request const& request = data.getRequest();
	ErrorPages const& errorPages = data.getErrorPages();

	try {
		// Create pipes
		SafePipe stdinPipe("CGI stdin"); // non-block
		SafePipe stdoutPipe("CGI stdout"); // non-blocking
		SafePipe stderrPipe("CGI stderr"); // non-block

		// Fork
		pid_t pid = fork();
		if (pid == -1) {
			errorFromClientFd(clientFd, "internal_server_error", request, errorPages, "CGI fork failed for client " + utils::str(clientFd));
			return; // pipes destructors clean automatically
		}
		// Execute
		if (pid == 0) // Child process
			_setupChildProcess(stdinPipe, stdoutPipe, stderrPipe, data);
		else // Parent process
			_setupParentProcess(stdinPipe, stdoutPipe, stderrPipe, clientFd, pid, data);
	} catch(std::exception& e) {
		errorFromClientFd(clientFd, "internal_server_error", request, errorPages, e.what());
		return; // pipes destructors clean automatically
	}
	*/
}

//TODO
void CgiHandler::_setupChildProcess(SafePipe& stdinPipe, SafePipe& stdoutPipe, SafePipe& stderrPipe, CgiData const& cgiParams)
{
	// Close manually useless pipe ends in child (SafePipe destructors will not be called)
    stdoutPipe.closeRead();
    stderrPipe.closeRead();
    stdinPipe.closeWrite();

    // Redirect to standard fds
    if (dup2(stdinPipe.readFd(), STDIN_FILENO) == -1) _exit(1);
    if (dup2(stdoutPipe.writeFd(), STDOUT_FILENO) == -1) _exit(1);
    if (dup2(stderrPipe.writeFd(), STDERR_FILENO) == -1) _exit(1);

	// Close manually previous fds after redirection (SafePipe destructors will not be called)
    stdinPipe.closeRead();
    stdoutPipe.closeWrite();
    stderrPipe.closeWrite();

    // Execute CGI (at this point all fds are closed; only 0, 1, 2 stay open)
    execve(cgiParams.getExecutor().c_str(), cgiParams.getArgv().data(), cgiParams.getEnvp().data());
    _exit(1); // execve failed
}

//TODO
void CgiHandler::_setupParentProcess(SafePipe& stdinPipe, SafePipe& stdoutPipe, SafePipe& stderrPipe, int clientFd, pid_t pid, CgiData const& data)
{
    // Écrire le body de la requête
	Request const& req = data.getRequest();
    std::string const& method = req.getMethod();
    if (method == "POST" || method == "PUT") {
        std::string const& body = req.getBody();
        if (!body.empty()) {
            ssize_t written = write(stdinPipe.writeFd(), body.c_str(), body.size());
            if (written == -1) {
                Log::prod("warning", "Failed to write request body to CGI stdin");
            }
        }
    }
    // Fermer stdinWrite car on a fini d'écrire
    close(stdinPipe.writeFd());

    // Créer le contexte avec les fd de lecture seulement
    CgiContext* ctx = new CgiContext(pid, clientFd, stdinPipe, stdoutPipe, stderrPipe, data);
    _contextsByPid[pid] = ctx;
    _contextsByPipeFd[stdoutPipe.readFd()] = ctx;
    _contextsByPipeFd[stderrPipe.readFd()] = ctx;

    // Ajouter à epoll
    if (_network) {
        _network->epollControl(stdoutPipe.readFd(), EPOLL_CTL_ADD, EPOLLIN, "CGI stdout pipe");
        _network->epollControl(stderrPipe.readFd(), EPOLL_CTL_ADD, EPOLLIN, "CGI stderr pipe");
    }

    Log::dev("event", "CGI launched async (pid " + utils::str(pid) + ") for client " + Log::hl(clientFd));
}

// TODO
void	CgiHandler::handlePipeRead(int pipeFd)
{
	(void)pipeFd;
	Log::dev("todo", "CgiHandler::handlePipeRead"); // DEBUG
}

/**
 * Does not clean up ressources.
 */
void CgiHandler::errorFromClientFd(int clientFd, std::string const& errorSlug, Request const& request, ErrorPages const& errorPages, std::string const& errorMsg)
{
	Log::prod("error", errorMsg);
	Response errorResp = StaticHandler::error(errorSlug, request, errorPages);
	errorResp.setHeader("Connection", "close"); // force close
	if (_network)
		_network->sendResponse(clientFd, errorResp, false);
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

	// Cleanup ressources (pipes, context)
	_contextsByPid.erase(it);
	if (!ctx)
		return;
	int stdoutReadFd = ctx->getStdoutPipe().readFd();
	if (stdoutReadFd != -1) {
		_contextsByPipeFd.erase(stdoutReadFd);
		if (_network)
			_network->epollControl(stdoutReadFd, EPOLL_CTL_DEL, 0, "CGI cleanup stdout"); // TODO catch
		close(stdoutReadFd);
	}
	int	stderrReadFd = ctx->getStderrPipe().readFd();
	if (stderrReadFd != -1) {
		_contextsByPipeFd.erase(stderrReadFd);
		if (_network)
			_network->epollControl(stderrReadFd, EPOLL_CTL_DEL, 0, "CGI cleanup stderr"); // TODO catch
		close(stderrReadFd);
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
void	CgiHandler::_finalizeResponse(CgiContext* ctx, int exitStatus)
{
	(void)ctx, (void)exitStatus;
	Log::dev("debug", "CgiHandler::_finalizeResponse"); // DEBUG
}

void	CgiHandler::_addPipeToEpoll(int fd, std::string const& context)
{
	if (_network)
		_network->epollControl(fd, EPOLL_CTL_ADD, EPOLLIN, context); // TODO catch
}
