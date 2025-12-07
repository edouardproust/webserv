#include "cgi/CgiHandler.hpp"
#include "network/Network.hpp"

size_t const	CgiHandler::_TIMEOUT_SECONDS = 30; // for big uploads
size_t const	CgiHandler::_READ_BUFFER_SIZE = 4096;
size_t const	CgiHandler::_LINUX_PIPE_BUFFER_SIZE = 65536;

CgiHandler::CgiHandler(Network* network)
: _network(network)
{}

CgiHandler::~CgiHandler()
{
	fullCleanup();
}

void CgiHandler::launchAsync(int clientFd, Response& resp)
{
	CgiData* d = resp.transferCgiDataOwnership();
	if (!CGI_ENABLED) {
		_sendErrorResponse("not_implemented", clientFd, d, "CGI not implemented.");
		delete d;
		return;
	}

	Log::dev("debug", "Starting CGI for client fd " + Log::hl(clientFd));

	// Create pipes and context
	CgiContext* ctx = _createCgiContext(clientFd, d);
	if (ctx == NULL) {
		_sendErrorResponse("internal_error", clientFd, d, "Failed to create CGI context.");
		return;
	}

	// Fork and execute
	pid_t pid;
	if (!_safeFork(ctx, pid))
		return;
	if (pid == 0) // Child process
		_executeChildProcess(ctx, d);
	else if (pid > 0) // Parent process
		_setupParentProcess(pid, ctx);
}

CgiContext* CgiHandler::_createCgiContext(int clientFd, CgiData* d)
{
	int pipeIn[2] = {-1, -1};
	int pipeOut[2] = {-1, -1};
	int pipeErr[2] = {-1, -1};

	if (pipe(pipeIn) == -1) {
		Log::prod("error", "Failed to create stdin pipe: " + std::string(strerror(errno)));
		return NULL;
	}
	if (pipe(pipeOut) == -1) {
		Log::prod("error", "Failed to create stdout pipe: " + std::string(strerror(errno)));
		CgiContext::closePipe(pipeIn);
		return NULL;
	}
	if (pipe(pipeErr) == -1) {
		Log::prod("error", "Failed to create stderr pipe: " + std::string(strerror(errno)));
		CgiContext::closePipe(pipeIn);
		CgiContext::closePipe(pipeOut);
		return NULL;
	}

	// Create context
	CgiContext* ctx = new CgiContext(
		clientFd, d,
		pipeIn[0], pipeIn[1],
		pipeOut[0], pipeOut[1],
		pipeErr[0], pipeErr[1]
	);

	ctx->setStartTime();
	Log::dev("debug", "Created CgiContext:\n" + utils::str(*ctx));

	return ctx;
}

bool	CgiHandler::_safeFork(CgiContext* ctx, pid_t& outPid)
{
	outPid = fork();
	if (outPid == -1) {
		Log::prod("error", "fork() failed: " + std::string(strerror(errno)));
		ctx->closeAllPipes(); // Cleanup all the pipes
		_sendErrorResponse("internal_error", ctx->getClientFd(), ctx->getCgiData(), "CGI fork failed.");
		delete ctx;
		return false;
	}
	return true;
}

void	CgiHandler::_executeChildProcess(CgiContext* ctx, CgiData const* d)
{
	// Close parent's ends of pipes
	ctx->closeInWriteFd();  // parent writes here
	ctx->closeOutReadFd();  // parent reads here
	ctx->closeErrReadFd();  // parent reads here

	// Redirect stdin/stdout/stderr to pipes
	if (dup2(ctx->getInReadFd(), STDIN_FILENO) == -1 ||
		dup2(ctx->getOutWriteFd(), STDOUT_FILENO) == -1 ||
		dup2(ctx->getErrWriteFd(), STDERR_FILENO) == -1) {
		perror("dup2 failed");
		_exit(1);
	}

	// Close now-useless pipe fds (no need to set them on -1)
	ctx->closeInReadFd();
	ctx->closeOutWriteFd();
	ctx->closeErrWriteFd();

	// Execute CGI
	execve(d->getExecutor().data(), d->getArgv().data(), d->getEnvp().data());

	// If we reach here, execve failed
	perror("execve failed");
	_exit(1);
}

void	CgiHandler::_setupParentProcess(pid_t pid, CgiContext* ctx)
{
	ctx->setPid(pid);
	Log::dev("cgi", "Parent process (child process pid: " + Log::hl(ctx->getPid()) + ").");

	// Close child's ends of pipes
	ctx->closeInReadFd();   // child reads here
	ctx->closeOutWriteFd(); // child writes here
	ctx->closeErrWriteFd(); // child writes here

	// Set pipes non-blocking
    Socket::setNonBlocking(ctx->getInWriteFd(), false);
    Socket::setNonBlocking(ctx->getOutReadFd(), false);
    Socket::setNonBlocking(ctx->getErrReadFd(), false);

	// Register stdout and stderr pipes in epoll
	_network->epollControl(ctx->getOutReadFd(), EPOLL_CTL_ADD, EPOLLIN, "CGI stdout pipe");
	_network->epollControl(ctx->getErrReadFd(), EPOLL_CTL_ADD, EPOLLIN, "CGI stderr pipe");

	// Store context in maps
	_contextsByPid[pid] = ctx;
	_contextsByPipeFd[ctx->getOutReadFd()] = ctx;
	_contextsByPipeFd[ctx->getErrReadFd()] = ctx;

	// Setup stdin pipe if there's a body to send
	_setupStdinPipe(ctx);

	Log::dev("cgi", "Context stored, pipes ready.");
}

void	CgiHandler::_setupStdinPipe(CgiContext* ctx)
{
	std::string const& body = ctx->getRequest().getBody();

	if (!body.empty()) {
		Log::dev("cgi", "Will send " + Log::hl(body.size()) + " bytes to CGI stdin");

		// Register stdin pipe in epoll for writing
		int inWriteFd = ctx->getInWriteFd();
		if (inWriteFd == -1) {
			Log::dev("warning", "stdin write fd already closed, cannot setup pipe");
			return;
		}
		_network->epollControl(ctx->getInWriteFd(), EPOLL_CTL_ADD, EPOLLOUT, "CGI stdin pipe");
		_contextsByPipeFd[ctx->getInWriteFd()] = ctx;
	}

	// No body: close stdin immediately
	else {
		ctx->closeInWriteFd();
	}
}

void	CgiHandler::writeCgiInput(int pipeFd)
{
	if (OPTIMIZED_READ_WRITE)
		_writeCgiInputOptimized(pipeFd);
	else
		_writeCgiInputStrict(pipeFd);
}

void	CgiHandler::_writeCgiInputStrict(int pipeFd)
{
	std::map<int, CgiContext*>::iterator it = _contextsByPipeFd.find(pipeFd);
	if (it == _contextsByPipeFd.end()) {
		Log::dev("warning", "writeCgiInput: unknown pipeFd " + utils::str(pipeFd));
		return;
	}

	CgiContext* ctx = it->second;

	// Check if client is still connected
	if (!_network->isClientConnected(ctx->getClientFd())) {
        _closeStdinPipe(ctx, pipeFd, "Client " + utils::str(ctx->getClientFd()) + " disconnected during stdin write, aborting");
        return;
    }

	std::string const& body = ctx->getRequest().getBody();
	size_t sent = ctx->getInputBytesSent();
	size_t remaining = body.size() - sent;

	// All input already sent (should not happen)
	if (remaining == 0) {
		_closeStdinPipe(ctx, pipeFd, "All input already sent, closing stdin pipe");
        return;
	}

	// Else, write the remaining input in stdin
	ssize_t written = write(pipeFd, body.data() + sent, remaining);
	if (written > 0) {
		ctx->addInputBytesSent(written);
		Log::dev("cgi", "Wrote " + Log::hl(written) + " bytes to CGI stdin (" + Log::hl(ctx->getInputBytesSent()) + "/" + utils::str(body.size()) + ")");

		// Check if all input has been sent
		if (ctx->getInputBytesSent() >= body.size()) {
			_closeStdinPipe(ctx, pipeFd, "All input sent to CGI executable, closing stdin pipe");
		} // Else, wait for next EPOLLOUT to send more
	} else if (written == 0) { // EOF on pipe
		_closeStdinPipe(ctx, pipeFd, "write() returned 0, CGI closed stdin");
	} else {
		// Prpbably EAGAIN -> Wait next EPOLLOUT
	}
}

void CgiHandler::_writeCgiInputOptimized(int pipeFd)
{
    std::map<int, CgiContext*>::iterator it = _contextsByPipeFd.find(pipeFd);
    if (it == _contextsByPipeFd.end()) {
        Log::dev("warning", "writeCgiInput: unknown pipeFd " + utils::str(pipeFd));
        return;
    }
    CgiContext* ctx = it->second;
    // Check if client is still connected
    if (!_network->isClientConnected(ctx->getClientFd())) {
        _closeStdinPipe(ctx, pipeFd, "Client disconnected during stdin write");
        return;
    }

    std::string const& body = ctx->getRequest().getBody();
    size_t sent = ctx->getInputBytesSent();
    while (sent < body.size()) {
        size_t remaining = body.size() - sent;
        size_t toWrite = std::min(remaining, _LINUX_PIPE_BUFFER_SIZE); // Write up to pipe buffer size
        ssize_t written = write(pipeFd, body.data() + sent, toWrite);
        if (written > 0) {
            sent += written;
            ctx->setInputBytesSent(sent);
            if (sent % Const::READ_WRITE_LOG_THRESHOLD_SIZE == 0 || sent == body.size()) {  // Log every 1MB to prevent log spam
                Log::dev("cgi", "Wrote " + Log::hl(sent) + "/" + utils::str(body.size()) + " bytes to CGI stdin.");
            }
            if (written < (ssize_t)toWrite) {
				// Stdin pipe buffer is full -> wait for EPOLLOUT (CGI will read and free space in pipe)
                return;
            }
            // Continue to write...
        } else { // written <= 0: pipe is still full (EAGAIN) or error
            if (written == 0) { // Shouldn't happen (error) -> cleanup
                _closeStdinPipe(ctx, pipeFd, "write() returned 0");
            }
            // written == -1: pipe is still full (EAGAIN) -> Wait more for EPOLLOUT
            return;
        }
    }

    // All the data was sent
    _closeStdinPipe(ctx, pipeFd, "All " + Log::hl(body.size()) + " bytes sent to CGI stdin.");
}

void CgiHandler::readCgiOutput(int pipeFd)
{
	std::map<int, CgiContext*>::iterator it = _contextsByPipeFd.find(pipeFd);
	if (it == _contextsByPipeFd.end()) {
		Log::dev("warning", "readAndAccumulateCgiOutput: unknown pipeFd " + utils::str(pipeFd));
		return;
	}

	CgiContext* ctx = it->second;
	char buffer[_READ_BUFFER_SIZE];
	ssize_t bytesRead = read(pipeFd, buffer, _READ_BUFFER_SIZE);

	// If we received some data
	if (bytesRead > 0) {
		if (pipeFd == ctx->getOutReadFd()) {
			ctx->appendOutput(buffer, bytesRead);
			if (ctx->getOutput().size() % Const::READ_WRITE_LOG_THRESHOLD_SIZE == 0)
				Log::dev("cgi", "Read " + Log::hl(ctx->getOutput().size()) + " bytes from stdout (pipe fd " + Log::hl(pipeFd) + ")");
		} else if (pipeFd == ctx->getErrReadFd()) {
			ctx->appendError(buffer, bytesRead);
			Log::dev("cgi", "Read " + utils::str(bytesRead) + " bytes from stderr");
		}
	}

	// If we get EOF (pipe was closed)
	else if (bytesRead == 0) {
		Log::dev("cgi", "EOF on pipe fd " + utils::str(pipeFd));
		_network->epollControl(pipeFd, EPOLL_CTL_DEL, 0, "CGI pipe EOF"); // remove fd from epoll
		_contextsByPipeFd.erase(pipeFd); // remove from map BEFORE fd becomes -1 (below)
		// mark which fd was closed to prevent race condition in checkCompletion()
		if (pipeFd == ctx->getOutReadFd()) // fd of stdout was closed
			ctx->closeOutReadFd();
		else if (pipeFd == ctx->getErrReadFd()) { // fd of stderr was closed
			// Log error if not empty
			if (!ctx->getError().empty()) {
            	Log::prod("warning", "CGI stderr (" + utils::str(ctx->getError().size()) + " bytes):\n" + ctx->getError());
			}
			ctx->closeErrReadFd();
    	}
	} else { // bytesRead == -1: error
		Log::dev("debug", "read() returned -1 on pipe fd " + utils::str(pipeFd) + ": " + utils::str(strerror(errno)));
	}
}

/**
 * Check on each epoll_wait loop if any CGI Responses are ready to be finialized.
 * If so, finilize them.
 *
 * @note This method is non-blocking thanks to `WNOHANG`.
 */
void CgiHandler::checkCompletion()
{
	if (_contextsByPid.empty()) return;
	std::map<pid_t, CgiContext*>::iterator it = _contextsByPid.begin();
	time_t now = time(NULL);

	while (it != _contextsByPid.end()) {
		pid_t pid = it->first;
		CgiContext* ctx = it->second;

		// Check timeout
        time_t elapsed = now - ctx->getStartTime();
        if (elapsed > (time_t)_TIMEOUT_SECONDS) {
            _handleTimeout(ctx, elapsed);
            _contextsByPid.erase(it++);
            continue;
        }

		// Check if process has exited (waitpid is called repeatedly until process exits)
		if (!ctx->hasProcessExited()) {
			int status;
			pid_t result = waitpid(pid, &status, WNOHANG);
			if (result > 0) { // CGI execution finished
				ctx->setProcessExited(true);
				ctx->setExitStatus(WIFEXITED(status) ? WEXITSTATUS(status) : -1);
				Log::dev("cgi", "CGI process (pid " + Log::hl(pid) + ") exited with code " + Log::hl(ctx->getExitStatus()) + ".");
				// We keep pipes open and in epoll for reading CGI output
			}
		}

		// Check if we can finalize (process exited and both pipes closed)
		if (ctx->hasProcessExited() && ctx->isStdoutClosed() && ctx->isStderrClosed()) {
			Log::dev("debug", "CGI " + utils::str(pid) + " ready to finalize");

			// If client was disconnected via Network::disconnectCLient: cleanup CgiContext
			// (This is done ONLY once CGI process finished)
			if (!_network->isClientConnected(ctx->getClientFd())) {
				Log::dev("warning", "Client " + utils::str(ctx->getClientFd()) + " disconnected before CGI finished, discarding response");
				delete ctx;
				_contextsByPid.erase(it++);
				continue;
			}

			// Build and send response
			if (ctx->getExitStatus() != 0) { // CGI failed -> send corresponding error
				_sendErrorResponse("internal_error", ctx->getClientFd(), ctx->getCgiData(), "CGI process exited with code " + utils::str(ctx->getExitStatus()));
			} else { // CGI succeeded -> parse output
				try {
					Log::dev("debug", "CGI output:\n" + PrintableString(Log::excerpt(Log::EXCERPT_SIZE, ctx->getOutput())));
					Response resp;
					resp.parseFromCgiOutput(ctx->getOutput());
					resp.handleSession(ctx->getRequest());
					_network->prepareResponseSend(ctx->getClientFd(), resp);
					_network->epollControl(ctx->getClientFd(), EPOLL_CTL_MOD, EPOLLOUT, "CGI response ready");
				} catch (std::exception& e) {
					_sendErrorResponse("internal_error", ctx->getClientFd(), ctx->getCgiData(), "Failed to parse CGI output: " + utils::str(e.what()));
				}
			}

			// Cleanup
			delete ctx;
			_contextsByPid.erase(it++); // increment
		}

		// Not ready yet -> keep waiting
		else {
			++it; // increment
		}
	}
}

void	CgiHandler::_handleTimeout(CgiContext* ctx, time_t elapsedTime)
{
	Log::prod("warning", "CGI timeout: process " + Log::hl(ctx->getPid()) + " exceeded " + Log::hl(_TIMEOUT_SECONDS) + " seconds (closed after " + Log::hl(elapsedTime) + " sec).");
	pid_t pid = ctx->getPid();
	int clientFd = ctx->getClientFd();

	// Kill process
	kill(pid, SIGKILL);
	waitpid(pid, NULL, 0); // reap zombie
	Log::dev("close", "Killed CGI process " + utils::str(pid));

	// Send timeout builtin error page to client
	if (_network->isClientConnected(clientFd)) {
		_sendErrorResponse("gateway_timeout", clientFd, ctx->getCgiData(), "CGI timeout error");
	} else {
		Log::dev("debug", "Client already disconnected, skipping timeout response");
	}

	// Cleanup
	_cleanupPipes(ctx);
	delete ctx;
	// Don't do `_contextsByPid.erase(pid)` here (the caller does it)
}

bool	CgiHandler::isCgiPipe(int fd) const
{
	bool result = _contextsByPipeFd.find(fd) != _contextsByPipeFd.end();
	//Log::dev("debug", "Fd " + Log::hl(fd) + " " + (result ? "is" : "is not") + " a CGI pipe.");
	return result;
}

bool	CgiHandler::hasActiveCgi(int clientFd) const
{
	for (std::map<pid_t, CgiContext*>::const_iterator it = _contextsByPid.begin(); it != _contextsByPid.end(); ++it) {
		if (it->second->getClientFd() == clientFd)
			return true;
	}
	return false;
}

std::map<pid_t, CgiContext*> const&	CgiHandler::getContextsByPid() const
{
	return _contextsByPid;
}

// CLEANUP

/**
 * Close & remove all still open pipes from epoll.
 */
void	CgiHandler::_cleanupPipes(CgiContext* ctx)
{
	// stdout pipe
	int outFd = ctx->getOutReadFd();
	if (!ctx->isStdoutClosed() && _contextsByPipeFd.find(outFd) != _contextsByPipeFd.end()) {
		_network->epollControl(outFd, EPOLL_CTL_DEL, 0, "CGI cleanup outRead");
		_contextsByPipeFd.erase(outFd);  // ← Erase AVANT close
		ctx->closeOutReadFd();  // ← Close après
	}

	// stderr pipe
	int errFd = ctx->getErrReadFd();
	if (!ctx->isStderrClosed() && _contextsByPipeFd.find(errFd) != _contextsByPipeFd.end()) {
		_network->epollControl(errFd, EPOLL_CTL_DEL, 0, "CGI cleanup errRead");
		_contextsByPipeFd.erase(errFd);
		ctx->closeErrReadFd();
	}

	// stdin pipe
	int inFd = ctx->getInWriteFd();
	if (inFd != -1 && _contextsByPipeFd.find(inFd) != _contextsByPipeFd.end()) {
		_network->epollControl(inFd, EPOLL_CTL_DEL, 0, "CGI cleanup inWrite");
		_contextsByPipeFd.erase(inFd);
		ctx->closeInWriteFd();
	}

	Log::dev("close", "Cleaned up pipes for CGI process " + utils::str(ctx->getPid()));
}

void CgiHandler::_closeStdinPipe(CgiContext* ctx, int pipeFd, std::string const& reason)
{
	if (!reason.empty())
		Log::dev("cgi", reason);
	if (_contextsByPipeFd.find(pipeFd) != _contextsByPipeFd.end()) {
		_network->epollControl(pipeFd, EPOLL_CTL_DEL, 0, "CGI stdin close");
		_contextsByPipeFd.erase(pipeFd);
	}
	ctx->closeInWriteFd();
}

void CgiHandler::fullCleanup()
{
	if (_contextsByPid.empty())  // Protection against double call
		return;

	Log::dev("close", "Cleaning up " + utils::str(_contextsByPid.size()) + " CGI processes...");

	std::map<pid_t, CgiContext*>::iterator it = _contextsByPid.begin();
	while (it != _contextsByPid.end()) {
		pid_t pid = it->first;
		CgiContext* ctx = it->second;

		if (!ctx->hasProcessExited()) {
			Log::dev("close", "Killing CGI process " + utils::str(pid));
			kill(pid, SIGKILL);
			waitpid(pid, NULL, 0);
		}

		_cleanupPipes(ctx);
		delete ctx;
		++it;
	}

	_contextsByPid.clear();
	_contextsByPipeFd.clear();

	Log::prod("close", "All CGI processes cleaned up.");
}

void	CgiHandler::_sendErrorResponse(std::string const& status, int clientFd, CgiData const* d, std::string const& msg, bool forceClose)
{
	if (!msg.empty())
		Log::prod("error", msg);
	if (!_network->isClientConnected(clientFd)) {
		Log::dev("debug", "Client " + utils::str(clientFd) + " already disconnected, skipping error response");
		return;
	}
	Response errorResp = StaticHandler::error(status, d->getRequest(), d->getErrorPages());
	if (forceClose)
		errorResp.setHeader("Connection", "close");
	_network->prepareResponseSend(clientFd, errorResp);
	_network->epollControl(clientFd, EPOLL_CTL_MOD, EPOLLOUT, "Sending CGI response");
}

void	CgiHandler::errorFromPipeFd(int pipeFd, std::string const& errorSlug, std::string const& errorMsg)
{
	std::map<int, CgiContext*>::iterator it = _contextsByPipeFd.find(pipeFd);
	if (it == _contextsByPipeFd.end())
		return;
	CgiContext* ctx = it->second;
	_sendErrorResponse(errorSlug, ctx->getClientFd(), ctx->getCgiData(), errorMsg, true); // force close (critical error)
}

