#include "cgi/CgiHandler.hpp"
#include "network/Network.hpp"

size_t const	CgiHandler::_TIMEOUT_SECONDS = 5;
size_t const	CgiHandler::_READ_BUFFER_SIZE = 4096;

CgiHandler::CgiHandler(Network* network)
: _network(network)
{}

CgiHandler::~CgiHandler()
{}

// TODO
void	CgiHandler::launchAsync(int clientFd, Response const& resp)
{
	CgiData const& d = resp.getCgiData();

	if (!CGI_ENABLED) {
		_sendNotImplementedResponse(clientFd, d);
		return;
	}

	Log::dev("debug", "Starting CGI for client fd " + Log::hl(clientFd));

	// Create pipes
	int pipeIn[2];
	int pipeOut[2];
	int pipeErr[2];

	pipe(pipeIn);
	pipe(pipeOut);
	pipe(pipeErr);

	// Create CgiContext
	CgiContext* ctx = new CgiContext(
		clientFd, d,
		pipeIn[0], pipeIn[1],
		pipeOut[0], pipeOut[1],
		pipeErr[0], pipeErr[1]
	);
	ctx->setStartTime();
	Log::dev("debug", "Created CgiContext:\n" + utils::str(*ctx));

	// Fork
	pid_t pid = fork();

	// CHILD PROCESS =========================

	if (pid == 0) {
		Log::dev("debug", "Child process started");
		// Close ends that child doesn't use (parent uses them)
		close(pipeIn[1]); // parent writes here
        close(pipeOut[0]); // parent reads here
        close(pipeErr[0]); // parent reads here
		sleep(1); // DEBUG
		_exit(42); // for now, just exit with 42 // TODO
	}

	// PARENT PROCESS ========================

	ctx->setPid(pid);
	Log::dev("debug", "Parent process: child pid = " + utils::str(ctx->getPid()));

	// Close ends that parent doesn't use (child uses them)
	close(pipeIn[0]); // child reads here
	close(pipeOut[1]); // child writes here
	close(pipeErr[1]); // child writes here

	// Set pipes non-blocking
	fcntl(pipeOut[0], F_SETFL, O_NONBLOCK);
	fcntl(pipeErr[0], F_SETFL, O_NONBLOCK);

	// Register pipes in epoll
	_network->epollControl(pipeOut[0], EPOLL_CTL_ADD, EPOLLIN, "CGI stdout pipe");
	_network->epollControl(pipeErr[0], EPOLL_CTL_ADD, EPOLLIN, "CGI stderr pipe");

	// Store context
	_contextsByPid[pid] = ctx;
	_contextsByPipeFd[pipeOut[0]] = ctx;
	_contextsByPipeFd[pipeErr[0]] = ctx;
	Log::dev("debug", "Context stored, pipes ready on fds " + utils::str(pipeOut[0]) + " and " + utils::str(pipeErr[0]));

	// For now close stdin pipe (not used yet)
    close(pipeIn[1]); // TODO: used to send body to cgi
}

// TODO
/**
 * Check on each epoll_wait loop if any CGI Responses are ready to be finialized.
 * If so, finilize them.
 *
 * @note This method is non-blocking thanks to `WNOHANG`.
 */
void CgiHandler::checkCompletion()
{
	std::map<pid_t, CgiContext*>::iterator it = _contextsByPid.begin();

	while (it != _contextsByPid.end()) {
		pid_t pid = it->first;
		CgiContext* ctx = it->second;
		int status;
		pid_t result = waitpid(pid, &status, WNOHANG);
		if (result > 0) {
			// Process terminé
			int exitStatus = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
			Log::dev("debug", "CGI process " + utils::str(pid) + " finished with exit code " + utils::str(exitStatus));
			// TODO: _finalizeResponse(ctx, exitStatus);
			// Cleanup and remove context
			delete ctx;
			_contextsByPid.erase(it++);
		} else {
			++it;
		}
	}
}

void CgiHandler::readAndAccumulateCgiOutput(int pipeFd)
{
	std::map<int, CgiContext*>::iterator it = _contextsByPipeFd.find(pipeFd);
	if (it == _contextsByPipeFd.end()) {
		Log::dev("warning", "readAndAccumulateCgiOutput: unknown pipeFd " + utils::str(pipeFd));
		return;
	}

	CgiContext* ctx = it->second;
	char buffer[_READ_BUFFER_SIZE];
	ssize_t bytesRead = read(pipeFd, buffer, _READ_BUFFER_SIZE);

	if (bytesRead > 0) { // data received
		if (pipeFd == ctx->getOutReadFd()) {
			ctx->appendOutput(buffer, bytesRead);
			Log::dev("cgi", "Read " + utils::str(bytesRead) + " bytes from stdout");
		} else if (pipeFd == ctx->getErrReadFd()) {
			ctx->appendError(buffer, bytesRead);
			Log::dev("cgi", "Read " + utils::str(bytesRead) + " bytes from stderr");
		}
	} else if (bytesRead == 0) { // EOF (pipe closed)
		Log::dev("debug", "EOF on pipe fd " + utils::str(pipeFd));
		_network->epollControl(pipeFd, EPOLL_CTL_DEL, 0, "CGI pipe EOF");
		close(pipeFd);
		_contextsByPipeFd.erase(pipeFd);
	} else { // bytesRead == -1: error
		Log::dev("debug", "read() returned -1 on pipe fd " + utils::str(pipeFd) + ": " + utils::str(strerror(errno)));
	}
}

bool	CgiHandler::isCgiPipe(int fd) const
{
	bool result = _contextsByPipeFd.find(fd) != _contextsByPipeFd.end();
	Log::dev("debug", "Fd " + Log::hl(fd) + " " + (result ? "is" : "is not") + " a CGI pipe.");
	return result;
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

// BUILTIN RESPONSES

void	CgiHandler::errorFromPipeFd(int pipeFd, std::string const& errorSlug, std::string const& errorMsg)
{
	std::map<int, CgiContext*>::iterator it = _contextsByPipeFd.find(pipeFd);
	if (it == _contextsByPipeFd.end())
		return;
	CgiContext* ctx = it->second;
	Log::prod("error", "CGI: " + errorMsg);
	Response errorResp = StaticHandler::error(errorSlug, ctx->getRequest(), ctx->getErrorPages());
	errorResp.setHeader("Connection", "close"); // force close
	_network->prepareResponseSend(ctx->getClientFd(), errorResp);
	_network->epollControl(ctx->getClientFd(), EPOLL_CTL_MOD, EPOLLOUT, "error response after CGI disabled");
}

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

