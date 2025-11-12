#include "cgi/CgiHandler.hpp"

CgiHandler::CgiHandler() {}

CgiHandler::~CgiHandler() {}

CgiHandler::ExecException::ExecException(std::string const& msg)
: std::runtime_error(msg) {}

CgiHandler::TimeoutException::TimeoutException(std::string const& msg)
: std::runtime_error(msg) {}

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
Response	CgiHandler::run(Request const& req, LocationBlock const* loc, std::string const& scriptName) {
	_scriptName = scriptName;
	_extension = utils::getFileExtension(_scriptName);
	_executor = loc->getCgiExecutor(_extension);
	if (pipe(_stdinPipe) == -1 || pipe(_stdoutPipe) == -1 || pipe(_stderrPipe) == -1)
		throw ExecException("pipe() failed");

	_buildEnvp(req, loc->getRoot());
	_buildArgv();
	pid_t pid = _forkAndExec();

	_prepareIo(req.getMethod(), req.getBody());

	int status = 0;
	useconds_t step = 10000; // 10ms
    useconds_t waited = 0;
    while (waited < utils::secondsToMicroseconds(CGI_TIMEOUT)) {
		pid_t ret = waitpid(pid, &status, WNOHANG);
		if (ret == 0) {
            usleep(step); // wait
            waited += step;
        } else if (ret == pid) {
			_readAllPipes();
			if (DEVMODE) std::cout << *this <<std::endl;
            return _handleStatus(status); // throw
		} else {
            throw ExecException("waitpid failed");
		}
	}
	// Timeout reached
    kill(pid, SIGKILL);
    waitpid(pid, &status, 0); // clean up
    throw TimeoutException("CGI timeout after " + utils::toString(waited) + " seconds");
}

pid_t	CgiHandler::_forkAndExec() const {
	pid_t pid = fork();
	if (pid == -1)
		throw ExecException("fork() failed");

	if (pid == 0) { // Child
		_redirectIoInChild();
		std::vector<char*> envp = _toCharPtrArray(_envp);
		std::vector<char*> argv = _toCharPtrArray(_argv);
		execve(_executor.c_str(), argv.data(), envp.data());
		_exit(1); // called only if execve failed
	}
	return pid;
}

void	CgiHandler::_prepareIo(std::string const& method, std::string const& reqBody) {
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

void	CgiHandler::_readAllPipes() {
	char buffer[READ_BUFFER];
	ssize_t n;
	while ((n = read(_stdoutPipe[0], buffer, sizeof(buffer))) > 0)
		_cgiOutput.append(buffer, n);
	close(_stdoutPipe[0]);
	while ((n = read(_stderrPipe[0], buffer, sizeof(buffer))) > 0)
		_cgiError.append(buffer, n);
	if (!_cgiError.empty())
		_cgiError = _cgiError.substr(0, _cgiError.find('\n')); // keep only first line without linebreak
	close(_stderrPipe[0]);
}

Response	CgiHandler::_handleStatus(int status) const {
	if (WIFEXITED(status)) {
		if (!_cgiOutput.empty()) {
			Response res(_cgiOutput); // throw Response::RawException (raw response invalid syntax)
			if (!_cgiError.empty()) // warning
				std::cerr << FT_WARNING << "CGI (" << _executor << "): " << _cgiError << std::endl;
			return res;
		}
		int exitCode = WEXITSTATUS(status);
		if (exitCode != 0) // error
			throw ExecException(_cgiError.empty() ? ("Error code " + utils::toString(exitCode)) : _cgiError);
		throw ExecException("exited with no output"); // improbable fallback
	} else if (WIFSIGNALED(status)) {
		int signal = WTERMSIG(status);
		throw ExecException("process killed by signal " + utils::toString(signal));
	}
	throw ExecException("unknown termination status"); // improbable fallback
}

// TODO: replace _error ? (It is not listed in allowed function of the subject)
void	CgiHandler::_redirectIoInChild() const {
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

void	CgiHandler::_buildEnvp(Request const& req, std::string const& locRoot) {
	_envp.clear(); // security
	std::map<std::string, std::string> tmp;
	// Essential CGI environment variables
	tmp["REQUEST_METHOD"] = req.getMethod();
	tmp["SCRIPT_FILENAME"] = _scriptName; // absolute path
	tmp["SCRIPT_NAME"] = req.getScriptName(); // relative path
    tmp["PATH_INFO"] = req.getPathInfo();
	tmp["QUERY_STRING"] = req.getQueryString();
	tmp["CONTENT_TYPE"] = req.getContentType();
	tmp["CONTENT_LENGTH"] = utils::toString(req.getBody().length());
	tmp["DOCUMENT_ROOT"] = locRoot;
	// Server protocol information
	tmp["GATEWAY_INTERFACE"] = "CGI/1.1";
	tmp["SERVER_PROTOCOL"] = req.getVersion();
	tmp["SERVER_SOFTWARE"] = SERVER_SOFTWARE;
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

void	CgiHandler::_buildArgv() {
    _argv.clear(); // security
	_argv.push_back(_executor); // argv[0] = path of the executable (/usr/bin/php-cgi, /usr/bin/python3, etc.)
	_argv.push_back(_scriptName); // argv[1] = script (/var/www/index.php, /var/www/website/script.py, etc)
}

std::string const&	CgiHandler::getScriptName() const {
	return _scriptName;
}

std::string const&	CgiHandler::getExecutor() const {
	return _executor;
}

std::vector<std::string> const&	CgiHandler::getEnvp() const {
	return _envp;
}

std::vector<std::string> const&	CgiHandler::getArgv() const {
	return _argv;
}

std::string const&	CgiHandler::getCgiOutput() const {
	return _cgiOutput;
}

std::string const&	CgiHandler::getCgiError() const {
	return _cgiError;
}

std::ostream&	operator<<(std::ostream& os, CgiHandler const& rhs) {
	os << "CgiHandler:\n";
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
	os << "- CGI raw response:\n[" << utils::excerpt(EXCERPT_LENGTH, rhs.getCgiOutput()) << "]\n";

	return os;
}

// static utils

std::string	CgiHandler::_headerToEnvVar(const std::string& headerName) {
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

std::vector<char*>	CgiHandler::_toCharPtrArray(const std::vector<std::string>& src) {
	std::vector<char*> result;
	result.reserve(src.size() + 1);
	for (size_t i = 0; i < src.size(); ++i)
		result.push_back(const_cast<char*>(src[i].c_str()));
	result.push_back(NULL);
	return result;
}
