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
		Log::dev("cgi", "Child process started.");

		// Close parent's ends of pipes
		close(pipeIn[1]); // parent writes here
		close(pipeOut[0]); // parent reads here
		close(pipeErr[0]); // parent reads here

		// Redirect stdin/stdout/stderr to pipes
		dup2(pipeIn[0], STDIN_FILENO);
		dup2(pipeOut[1], STDOUT_FILENO);
		dup2(pipeErr[1], STDERR_FILENO);

		// Close now-useless pipe fds
		close(pipeIn[0]);
		close(pipeOut[1]);
		close(pipeErr[1]);

		// Execute CGI
		execve(d.getExecutor().data(), d.getArgv().data(), d.getEnvp().data());

		// Si on arrive ici, execve a échoué
		perror("execve failed");
		_exit(1);
	}

	// PARENT PROCESS ========================

	ctx->setPid(pid);
	Log::dev("cgi", "Parent process (child process pid: " + Log::hl(ctx->getPid()) + ").");

	// Close child's ends of pipes
	close(pipeIn[0]); // child reads here
	close(pipeOut[1]); // child writes here
	close(pipeErr[1]); // child writes here

	// Set pipes non-blocking
	fcntl(pipeIn[1], F_SETFL, O_NONBLOCK);
	fcntl(pipeOut[0], F_SETFL, O_NONBLOCK);
	fcntl(pipeErr[0], F_SETFL, O_NONBLOCK);

	// Register outPipe and errPipe in epoll
	_network->epollControl(pipeOut[0], EPOLL_CTL_ADD, EPOLLIN, "CGI stdout pipe");
	_network->epollControl(pipeErr[0], EPOLL_CTL_ADD, EPOLLIN, "CGI stderr pipe");

	// Store context
	_contextsByPid[pid] = ctx;
	_contextsByPipeFd[pipeOut[0]] = ctx;
	_contextsByPipeFd[pipeErr[0]] = ctx;

	// Send request body to CGI stdin (if any)
	std::string const& body = d.getRequest().getBody();
	if (!body.empty()) {
		Log::dev("cgi", "Will send " + Log::hl(body.size()) + " bytes to CGI stdin");
		// Register inPipe pipe in epoll
		_network->epollControl(pipeIn[1], EPOLL_CTL_ADD, EPOLLOUT, "CGI stdin pipe");
		// Store context
    	_contextsByPipeFd[pipeIn[1]] = ctx;
	} else { // No data to read: close read end of stdin pipe
		close(pipeIn[1]);
	}

	Log::dev("cgi", "Context stored, pipes ready.");
}


void	CgiHandler::writeCgiInput(int pipeFd)
{
	std::map<int, CgiContext*>::iterator it = _contextsByPipeFd.find(pipeFd);
	if (it == _contextsByPipeFd.end()) {
		Log::dev("warning", "writeCgiInput: unknown pipeFd " + utils::str(pipeFd));
		return;
	}

	CgiContext* ctx = it->second;
	std::string const& body = ctx->getRequest().getBody();
	size_t sent = ctx->getInputBytesSent();
	size_t remaining = body.size() - sent;

	if (remaining == 0) { // All input already sent (should not happen)
		Log::dev("cgi", "All input already sent, closing stdin pipe");
		_network->epollControl(pipeFd, EPOLL_CTL_DEL, 0, "CGI stdin done");
		close(pipeFd);
		_contextsByPipeFd.erase(pipeFd);
		return;
	}

	// Write the remaining input
	ssize_t written = write(pipeFd, body.data() + sent, remaining);
	if (written > 0) {
		ctx->addInputBytesSent(written);
		//Log::dev("cgi", "Wrote " + Log::hl(written) + " bytes to CGI stdin (" + Log::hl(ctx->getInputBytesSent()) + "/" + utils::str(body.size()) + ")");

		// Check if all input has been sent
		if (ctx->getInputBytesSent() >= body.size()) {
			Log::dev("cgi", "All input sent, closing stdin pipe");
			_network->epollControl(pipeFd, EPOLL_CTL_DEL, 0, "CGI stdin done");
			close(pipeFd);
			_contextsByPipeFd.erase(pipeFd);
		}
		// Else, wait for next EPOLLOUT to send more
	} else {
		// written <= 0: wait for next EPOLLOUT or we have an error
		Log::dev("cgi", "write() returned " + utils::str(written) + ", waiting for next EPOLLOUT");
	}
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

	if (bytesRead > 0) { // data received
		if (pipeFd == ctx->getOutReadFd()) {
			ctx->appendOutput(buffer, bytesRead);
			Log::dev("cgi", "Read " + utils::str(bytesRead) + " bytes from stdout");

			// TODO stream response (chunked)
			// If headers not yet completely received, check for completion
			/*if (!ctx->headersReceived()) {
				std::pair<size_t, size_t> const& sep = utils::headersBodySeparatorPos(ctx->getOutput());
				if (sep.first != std::string::npos) { // Complete headers received
					ctx->setHeadersReceived(true);
					Log::dev("cgi", "CGI headers complete, preparing to stream response");
					_startStreamingResponse(ctx);
				}
			} else {
				// Headers already received: continue sending response
				_continueStreamingResponse(ctx);
			}
			*/
		} else if (pipeFd == ctx->getErrReadFd()) {
			ctx->appendError(buffer, bytesRead);
			Log::dev("cgi", "Read " + utils::str(bytesRead) + " bytes from stderr");
			Log::prod("warning", "CGI executable returned an error: " + std::string(buffer, bytesRead));
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

void CgiHandler::_startStreamingResponse(CgiContext* ctx)
{
	if (ctx->headersSent())
		return;

	// Parse CGI headers
	std::string const& output = ctx->getOutput();
	std::pair<size_t, size_t> const& sep = utils::headersBodySeparatorPos(output);
	if (sep.first == std::string::npos)
		return; // Headers not complete yet
	std::string headersPart = output.substr(0, sep.first);
	Response response;
	try {
		response.parseHeadersFromCgiOutput(headersPart);
		response.setHeader("Transfer-Encoding", "chunked"); // For streaming
	} catch (std::exception& e) {
		Log::prod("error", "Failed to parse CGI headers: " + utils::str(e.what()));
		return;
	}
	std::string httpResponse = response.stringify(true); // Built a raw headers string (without body)

	// Send to client
	_network->prepareResponseSend(ctx->getClientFd(), response);
	_network->epollControl(ctx->getClientFd(), EPOLL_CTL_MOD, EPOLLOUT, "CGI streaming start");
	ctx->setHeadersSent(true);
	Log::dev("cgi", "Streaming: HTTP headers sent to client");
}

void CgiHandler::_continueStreamingResponse(CgiContext* ctx)
{
	if (!ctx->headersSent())
		return; // Headers shoudl be sent first

	// Get chunk of the body that is already available
	std::string const& output = ctx->getOutput();
	std::pair<size_t, size_t> const& sep = utils::headersBodySeparatorPos(output);
	if (sep.first == std::string::npos)
		return;
	size_t bodyStart = sep.first + sep.second;
	std::string bodyChunk = output.substr(bodyStart);
	if (bodyChunk.empty())
		return; // No more to be send yet

	Log::dev("cgi", "Streaming: sending " + utils::str(bodyChunk.size()) + " bytes of body to client");
	// TODO: Envoyer le chunk au client
	// Pour l'instant, on va juste clear l'output après les headers
	// pour ne pas re-envoyer les mêmes données

	// On garde juste les headers dans output (pour pas les perdre)
	// et on vide le body déjà envoyé
	// ctx->clearOutputAfterHeaders(); // Nouvelle méthode à créer
}

/**
 * Check on each epoll_wait loop if any CGI Responses are ready to be finialized.
 * If so, finilize them.
 *
 * @note This method is non-blocking thanks to `WNOHANG`.
 */
void CgiHandler::checkCompletion()
{
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

		int status;
		pid_t result = waitpid(pid, &status, WNOHANG);
		if (result > 0) {
			// Process terminé
			int exitStatus = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
			Log::dev("debug", "CGI process " + utils::str(pid) + " finished with exit code " + utils::str(exitStatus));

			// Send response to client
			Response resp;
			if (exitStatus != 0) { // CGI failed (exit != 0) -> we send a "internal_server" error builtin page
				resp = StaticHandler::error("internal_error", ctx->getRequest(), ctx->getErrorPages());
			} else {
				try {
					Log::dev("debug", "CGI output:\n" + PrintableString(Log::excerpt(Log::EXCERPT_SIZE, ctx->getOutput())));
					resp.parseFromCgiOutput(ctx->getOutput());
				} catch (std::exception& e) {
					Log::prod("error", "Failed to parse CGI output: " + utils::str(e.what()));
					resp = StaticHandler::error("internal_error", ctx->getRequest(), ctx->getErrorPages());
				}
			}
			_network->prepareResponseSend(ctx->getClientFd(), resp);
			_network->epollControl(ctx->getClientFd(), EPOLL_CTL_MOD, EPOLLOUT, "CGI response ready");

			// Cleanup and remove context
			delete ctx;
			_contextsByPid.erase(it++);
		} else {
			++it;
		}
	}
}

void	CgiHandler::_handleTimeout(CgiContext* ctx, time_t elapsedTime)
{
	Log::prod("warning", "CGI timeout: process " + Log::hl(ctx->getPid()) + " exceeded " + Log::hl(_TIMEOUT_SECONDS) + " seconds (" + Log::hl(elapsedTime) + "sec).");

	// Kill process
	kill(ctx->getPid(), SIGKILL);
	Log::dev("close", "Killed CGI process " + utils::str(ctx->getPid()));

	// Send timeout builtin error page to clientt
	Response errorResp = StaticHandler::error("gateway_timeout", ctx->getRequest(), ctx->getErrorPages());
	_network->prepareResponseSend(ctx->getClientFd(), errorResp);
	_network->epollControl(ctx->getClientFd(), EPOLL_CTL_MOD, EPOLLOUT, "CGI timeout error");

	// Cleanup
	_cleanupPipes(ctx);
	delete ctx;
}

bool	CgiHandler::isCgiPipe(int fd) const
{
	bool result = _contextsByPipeFd.find(fd) != _contextsByPipeFd.end();
	//Log::dev("debug", "Fd " + Log::hl(fd) + " " + (result ? "is" : "is not") + " a CGI pipe.");
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

// CLEANUP

/**
 * Close & remove all still open pipes from epoll.
 */
void	CgiHandler::_cleanupPipes(CgiContext* ctx)
{
	// stdout pipe
	if (_contextsByPipeFd.find(ctx->getOutReadFd()) != _contextsByPipeFd.end()) {
		_network->epollControl(ctx->getOutReadFd(), EPOLL_CTL_DEL, 0, "CGI cleanup");
		close(ctx->getOutReadFd());
		_contextsByPipeFd.erase(ctx->getOutReadFd());
	}

	// stderr pipe
	if (_contextsByPipeFd.find(ctx->getErrReadFd()) != _contextsByPipeFd.end()) {
		_network->epollControl(ctx->getErrReadFd(), EPOLL_CTL_DEL, 0, "CGI cleanup");
		close(ctx->getErrReadFd());
		_contextsByPipeFd.erase(ctx->getErrReadFd());
	}

	// stdin pipe (si encore ouvert)
	if (_contextsByPipeFd.find(ctx->getInWriteFd()) != _contextsByPipeFd.end()) {
		_network->epollControl(ctx->getInWriteFd(), EPOLL_CTL_DEL, 0, "CGI cleanup");
		close(ctx->getInWriteFd());
		_contextsByPipeFd.erase(ctx->getInWriteFd());
	}

	Log::dev("close", "Cleaned up pipes for CGI process " + utils::str(ctx->getPid()));
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

