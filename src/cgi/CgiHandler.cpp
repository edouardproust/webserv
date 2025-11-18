#include "cgi/CgiHandler.hpp"

size_t const	CgiHandler::_TIMEOUT_MS = 5000; // milliseconds
size_t const	CgiHandler::_STEP_MS = 10; // milliseconds
size_t const	CgiHandler::_READ_BUFFER = 4096;

CgiHandler::CgiHandler() {
	_stdinPipe[0] = _stdinPipe[1] = -1;
	_stdoutPipe[0] = _stdoutPipe[1] = -1;
	_stderrPipe[0] = _stderrPipe[1] = -1;
}

CgiHandler::~CgiHandler()
{
	_cleanupPipes();
}

CgiHandler::ExecException::ExecException(std::string const& msg)
: std::runtime_error(msg)
{}

CgiHandler::TimeoutException::TimeoutException(std::string const& msg)
: std::runtime_error(msg)
{}

/**
 * Handle the CGI request, and returns a parsed Response object.
 *
 * Outputs:
 * - Throw a CgiHandler::ExecException if fork(), pipe(), dup2 fail, or if child process killed by signal
 * - Throw a Response::RawException if CGI raw reponse is invalid
 * - Else, parse the CGI raw response into a Response, then returns the later.
 *
 * Notes:
 * - Status code in the Response is set from the CGI raw response.
 */
Response CgiHandler::run(Request const& req, LocationBlock const* loc, std::string const& scriptName, HostPortPair const& listeningOn)
{
	try {
		_scriptName = scriptName;
		_extension = utils::getFileExtension(_scriptName);
		_executor = loc->getCgiExecutor(_extension);

		// Pipes
		if (pipe(_stdinPipe) == -1 || pipe(_stdoutPipe) == -1 || pipe(_stderrPipe) == -1)
			throw ExecException("pipe() failed");
		_setNonBlocking(_stdoutPipe[0]);
		_setNonBlocking(_stderrPipe[0]);

		// Run executable
		_buildEnvp(req, loc->getRoot(), listeningOn);
		_buildArgv();
		pid_t pid = _forkAndExec(); // Child process
		_prepareIo(req.getMethod(), req.getBody());

		// Timeout check
		int status = 0;
		if (!_waitWithTimeout(pid, status, _TIMEOUT_MS)) {
			kill(pid, SIGKILL);
			waitpid(pid, &status, 0); // Nettoyer le processus
			throw TimeoutException("CGI timeout after " + utils::str(_TIMEOUT_MS) + " ms");
		}

		// Read last data before and of process
		_readPipes();

		return _handleStatus(status);
	} catch (...) {
		_cleanupPipes();
		throw;
	}
}

void CgiHandler::_setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        throw ExecException("fcntl(F_GETFL) failed");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        throw ExecException("fcntl(F_SETFL) failed");
}

/**
 * Forks the current process and executes the CGI script in the child.
 *
 * Returns the PID of the child process.
 * Throws ExecException if fork() fails.
 */
pid_t	CgiHandler::_forkAndExec() const
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
 * Redirects stdin, stdout, and stderr in the child process to the corresponding pipes.
 *
 * Called only in the child process before execve.
 * Exits child process with _exit(1) if any dup2 fails.
 */
void	CgiHandler::_redirectIoInChild() const
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
 * Prepares the standard input/output/error pipes for the CGI process.
 *
 * Writes the request body to the CGI stdin if needed.
 * Closes unused pipe ends.
 */
void	CgiHandler::_prepareIo(std::string const& method, std::string const& reqBody)
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
 * Waits for the CGI child process to exit within a given timeout.
 *
 * Reads available stdout/stderr data during the wait loop.
 *
 * Returns true if the child exits before the timeout, false otherwise.
 * Throws ExecException if waitpid() fails.
 */
bool CgiHandler::_waitWithTimeout(pid_t pid, int& status, size_t timeout_ms)
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
 * Reads available data from stdout and stderr of the CGI process using select().
 *
 * Appends data to _cgiOutput and _cgiError.
 * Throws ExecException if select() fails.
 */
void CgiHandler::_readPipes()
{
	char buffer[_READ_BUFFER];
	ssize_t n;

	// Lecture non-bloquante de stdout
	while ((n = read(_stdoutPipe[0], buffer, sizeof(buffer))) > 0) {
		_cgiOutput.append(buffer, n);
	}
	if (n == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
		// Erreur réelle (pas juste "pas de données disponibles")
		throw ExecException("read from stdout failed");
	}

	// Lecture non-bloquante de stderr
	while ((n = read(_stderrPipe[0], buffer, sizeof(buffer))) > 0) {
		_cgiError.append(buffer, n);
	}
	if (n == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
		throw ExecException("read from stderr failed");
	}
}

/**
 * Checks the termination status of the CGI process and returns a Response.
 *
 * Throws ExecException if the process exited with error or was killed by signal.
 */
Response	CgiHandler::_handleStatus(int status)
{
	if (WIFEXITED(status)) {
		int exitCode = WEXITSTATUS(status);
		if (exitCode != 0) {
			if (!_cgiError.empty())
				_cgiError = _cgiError.substr(0, _cgiError.find('\n')); // keep only first line
			throw ExecException(_cgiError.empty() ? ("Error code " + utils::str(exitCode)) : _cgiError);
		}
		Response res(_cgiOutput); // throw Response::RawException if raw response invalid syntax
		return res;
	} else if (WIFSIGNALED(status)) {
		int signal = WTERMSIG(status);
		throw ExecException("process killed by signal " + utils::str(signal));
	}
	throw ExecException("unknown termination status"); // improbable fallback
}

void	CgiHandler::_cleanupPipes()
{
	if (_stdinPipe[0] != -1) close(_stdinPipe[0]);
    if (_stdinPipe[1] != -1) close(_stdinPipe[1]);
    if (_stdoutPipe[0] != -1) close(_stdoutPipe[0]);
    if (_stdoutPipe[1] != -1) close(_stdoutPipe[1]);
    if (_stderrPipe[0] != -1) close(_stderrPipe[0]);
    if (_stderrPipe[1] != -1) close(_stderrPipe[1]);

    _stdinPipe[0] = _stdinPipe[1] =
    _stdoutPipe[0] = _stdoutPipe[1] =
    _stderrPipe[0] = _stderrPipe[1] = -1;
}

/**
 * Builds the environment variables for the CGI script from the Request and LocationBlock.

 * Stores them in _envp.
 */
void	CgiHandler::_buildEnvp(Request const& req, std::string const& locRoot, HostPortPair const& listeningOn)
{
	_envp.clear(); // security
	std::map<std::string, std::string> tmp;
	// Essential CGI environment variables
	tmp["REQUEST_METHOD"] = req.getMethod();
	tmp["SCRIPT_FILENAME"] = _scriptName; // absolute path
	tmp["SCRIPT_NAME"] = req.getScriptName(); // relative path
	tmp["PATH_INFO"] = req.getPath().empty() ? "/" : req.getPath(); //full path for the tester to work
	tmp["PATH_TRANSLATED"] = locRoot + tmp["PATH_INFO"]; // TODO
	tmp["QUERY_STRING"] = req.getQueryString();
	tmp["CONTENT_TYPE"] = req.getContentType();
	tmp["CONTENT_LENGTH"] = utils::str(req.getBody().length());
	tmp["DOCUMENT_ROOT"] = locRoot;
	// Server protocol information
	tmp["GATEWAY_INTERFACE"] = "CGI/1.1";
	tmp["SERVER_PROTOCOL"] = req.getVersion();
	tmp["SERVER_SOFTWARE"] = Const::SERVER_SOFTWARE;
	// Additional variables from old subject requirements to pass ubuntu_tester
	tmp["AUTH_TYPE"] = "";  // TODO Authentication type (usually empty)
	tmp["REMOTE_ADDR"] = "127.0.0.1";  // TODO Client IP address
	tmp["REMOTE_IDENT"] = ""; // TODO Remote user identity (usually empty)
	tmp["REMOTE_USER"] = ""; // TODO Remote user name (usually empty)
	tmp["REQUEST_URI"] = req.getUri();
	tmp["SERVER_NAME"] = listeningOn.getHost();
	tmp["SERVER_PORT"] = utils::str(listeningOn.getPort());
	// HTTP headers (prefixed with HTTP_)
	Headers headers = req.getHeaders();
	for (Headers::const_iterator it = headers.begin(); it != headers.end(); ++it)
		tmp[_headerToEnvVar(it->first)] = it->second;
	// PHP specific variables
	if (_extension == ".php") {
		tmp["REDIRECT_STATUS"] = HttpStatus("ok").getCodeStr();
		tmp["PHP_SELF"] = req.getPath();
	}
	// Build "KEY=VALUE" strings in persistent storage
	for (std::map<std::string, std::string>::const_iterator it = tmp.begin(); it != tmp.end(); ++it)
		_envp.push_back(it->first + "=" + it->second);
}

/**
 * Builds the argument vector for execve, including the executor and script path.
 *
 * Stores them in _argv.
 */
void	CgiHandler::_buildArgv()
{
    _argv.clear(); // security
	_argv.push_back(_executor); // argv[0] = path of the executable (/usr/bin/php-cgi, /usr/bin/python3, etc.)
	_argv.push_back(_scriptName); // argv[1] = script (/var/www/index.php, /var/www/website/script.py, etc)
}

std::string const&	CgiHandler::getScriptName() const
{
	return _scriptName;
}

std::string const&	CgiHandler::getExecutor() const
{
	return _executor;
}

std::vector<std::string> const&	CgiHandler::getEnvp() const
{
	return _envp;
}

std::vector<std::string> const&	CgiHandler::getArgv() const
{
	return _argv;
}

std::string const&	CgiHandler::getCgiOutput() const
{
	return _cgiOutput;
}

std::string const&	CgiHandler::getCgiError() const
{
	return _cgiError;
}

std::ostream&	operator<<(std::ostream& os, CgiHandler const& rhs)
{
	os << "- script path: " << PrintableString(rhs.getScriptName()) << "\n";
	os << "- executor: " << PrintableString(rhs.getExecutor()) << "\n";

	std::vector<std::string> envp = rhs.getEnvp();
	os << "- envp: " << envp.size() << "\n";
	for (size_t i = 0; i < envp.size(); ++i)
		os << "  - " << PrintableString(envp[i]) << "\n";

	std::vector<std::string> argv = rhs.getArgv();
	os << "- argv: " << argv.size() << "\n";
	for (size_t i = 0; i < argv.size(); ++i)
		os << "  - " << PrintableString(argv[i]) << "\n";

	os << "- CGI error: " << PrintableString(rhs.getCgiError()) << "\n";
	os << "- CGI raw response:\n[" << Log::excerpt(Log::EXCERPT_CHARS, rhs.getCgiOutput()) << "]\n";

	return os;
}

// static utils

/**
 * Converts a header name (e.g., "Content-Type") into a CGI environment variable format
 * (e.g., "HTTP_CONTENT_TYPE").
 */
std::string	CgiHandler::_headerToEnvVar(const std::string& headerName)
{
	std::string result = "HTTP_";
	for (size_t i = 0; i < headerName.length(); ++i) {
		char c = headerName[i];
		if (c == '-')
			result += '_';
		else
			result += static_cast<char>(std::toupper(c));
	}
	return result;
}

/**
 * Converts a vector of std::string into a vector of char* suitable for execve.
 *
 * Adds a terminating NULL at the end.
 */
std::vector<char*>	CgiHandler::_toCharPtrArrayInChild(const std::vector<std::string>& src)
{
	std::vector<char*> result;
	result.reserve(src.size() + 1);
	for (size_t i = 0; i < src.size(); ++i)
		result.push_back(const_cast<char*>(src[i].c_str()));
	result.push_back(NULL);
	return result;
}
