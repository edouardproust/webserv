#include "cgi/PipeCgiHandler.hpp"

size_t const	PipeCgiHandler::_TIMEOUT_MS = 5000; // milliseconds
size_t const	PipeCgiHandler::_STEP_MS = 10; // milliseconds
size_t const	PipeCgiHandler::_READ_BUFFER = 4096;

PipeCgiHandler::PipeCgiHandler() {
	_initPipe(_stdinPipe);
	_initPipe(_stdoutPipe);
	_initPipe(_stderrPipe);
}

PipeCgiHandler::~PipeCgiHandler()
{
	_closePipe(_stdinPipe);
    _closePipe(_stdoutPipe);
    _closePipe(_stderrPipe);
}

/**
 * Handle the CGI request, and returns a parsed Response object.
 *
 * Outputs:
 * - Throw a PipeCgiHandler::ExecException if fork(), pipe(), dup2 fail, or if child process killed by signal
 * - Throw a Response::RawException if CGI raw reponse is invalid
 * - Else, parse the CGI raw response into a Response, then returns the later.
 *
 * Notes:
 * - Status code in the Response is set from the CGI raw response.
 */
Response PipeCgiHandler::execute(Request const& req, LocationBlock const* loc, std::string const& scriptName, HostPortPair const& listeningOn)
{
	_scriptName = scriptName;
	_extension = utils::getFileExtension(_scriptName);
	_executor = loc->getCgiExecutor(_extension);

	_prepareIo();

	_buildEnvp(req, loc->getRoot(), listeningOn);
	_buildArgv();


	// Run executable
	pid_t pid = _forkAndExec(); // Child process
	_writeBodyToPipe(req.getMethod(), req.getBody());

	// Timeout check
	int status = 0;
	if (!_waitWithTimeout(pid, status, _TIMEOUT_MS)) {
		kill(pid, SIGKILL);
		waitpid(pid, &status, 0); // Nettoyer le processus
		throw TimeoutException("CGI timeout after " + utils::str(_TIMEOUT_MS) + " ms");
	}

	// Read last data before end of process
	_readPipes();

	return _handleStatus(status);
}

void	PipeCgiHandler::_prepareIo()
{

		if (pipe(_stdinPipe) == -1 || pipe(_stdoutPipe) == -1 || pipe(_stderrPipe) == -1)
		throw ExecException("pipe() failed");
		_setNonBlocking(_stdoutPipe[0]);
		_setNonBlocking(_stderrPipe[0]);
}

/**
 * Forks the current process and executes the CGI script in the child.
 *
 * Returns the PID of the child process.
 * Throws ExecException if fork() fails.
 */
pid_t	PipeCgiHandler::_forkAndExec() const
{
	pid_t pid = fork();
	if (pid == -1)
		throw ExecException("fork() failed");

	if (pid == 0) { // Child
		_redirectIoInChild();
		std::vector<char*> envp = _toCharPtrArrayInChild(_envp);
		std::vector<char*> argv = _toCharPtrArrayInChild(_argv);
		execve(_executor.c_str(), argv.data(), envp.data());
		_exit(1); // called only if execve failed
	}
	return pid;
}

/**
 * Waits for the CGI child process to exit within a given timeout.
 *
 * Reads available stdout/stderr data during the wait loop.
 *
 * Returns true if the child exits before the timeout, false otherwise.
 * Throws ExecException if waitpid() fails.
 */
bool PipeCgiHandler::_waitWithTimeout(pid_t pid, int& status, size_t timeout_ms)
{
	size_t waited_ms = 0;
	while (waited_ms < timeout_ms) {
		// Check if process was finished
		int result = waitpid(pid, &status, WNOHANG);
		if (result == pid) {
			return true; // Processus terminé
		} else if (result == -1) {
			throw ExecException("waitpid failed");
		}
		// Read available data
		_readPipes();
		// Wait a bit before to try again
		usleep(_STEP_MS * 1000);
		waited_ms += _STEP_MS;
	}

	return false; // Timeout
}

/**
 * Writes the request body to the CGI stdin if needed and closes unused pipe ends.
 */
void	PipeCgiHandler::_writeBodyToPipe(std::string const& method, std::string const& reqBody)
{
	// close unused pipe ends
	close(_stdinPipe[0]); // reading body
	close(_stdoutPipe[1]); // writing stdout
	close(_stderrPipe[1]); // writing stderr

	// Write request body in CGI's stdin (if PUT/POST)
	if (method == "POST" || method == "PUT") {
		ssize_t n = write(_stdinPipe[1], reqBody.c_str(), reqBody.size());
		(void)n; // mute compiler error
	}
	close(_stdinPipe[1]); // stdin write
}

/**
 * Redirects stdin, stdout, and stderr in the child process to the corresponding pipes.
 *
 * Called only in the child process before execve.
 * Exits child process with _exit(1) if any dup2 fails.
 */
void	PipeCgiHandler::_redirectIoInChild() const
{
	// redirect stdin
	close(_stdinPipe[1]); // close writing
	if (dup2(_stdinPipe[0], STDIN_FILENO) == -1)
		_exit(1);
	close(_stdinPipe[0]);
	// redirect stdout and stderr
	close(_stdoutPipe[0]); // close reading
	close(_stderrPipe[0]);
	if (dup2(_stdoutPipe[1], STDOUT_FILENO) == -1)
		_exit(1);
	if (dup2(_stderrPipe[1], STDERR_FILENO) == -1) // to catch error messages
		_exit(1);
	close(_stdoutPipe[1]);
	close(_stderrPipe[1]);
}

/**
 * Reads available data from stdout and stderr of the CGI process using select().
 *
 * Appends data to _cgiOutput and _cgiError.
 * Throws ExecException if select() fails.
 */
void PipeCgiHandler::_readPipes()
{
	char buffer[_READ_BUFFER];
	ssize_t n;

	// Non-blocking reading of stdout
	while ((n = read(_stdoutPipe[0], buffer, sizeof(buffer))) > 0)
		_cgiOutput.append(buffer, n);
	if (n == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
		throw ExecException("read from stdout failed");

	// Non-blocking reading of stderr
	while ((n = read(_stderrPipe[0], buffer, sizeof(buffer))) > 0)
		_cgiError.append(buffer, n);
	if (n == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
		throw ExecException("read from stderr failed");
}
