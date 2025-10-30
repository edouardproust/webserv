#include "cgi/CgiHandler.hpp"

CgiHandler::CgiHandler() {}

CgiHandler::~CgiHandler() {}

CgiHandler::ExecException::ExecException(std::string const& msg)
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
Response	CgiHandler::run(Request const& req, LocationBlock const* loc, std::string const& filePath) {
	_filePath = filePath;
	_extension = utils::getFileExtension(_filePath);
	_executor = loc->getCgiExecutor(_extension);
	if (pipe(_stdinPipe) == -1 || pipe(_stdoutPipe) == -1 || pipe(_stderrPipe) == -1)
		throw ExecException("pipe() failed");

	_buildEnvp(req, loc->getRoot());
	_buildArgv();
	pid_t pid = _forkAndExec();
	_communicateWithChild(req);
	if (DEVMODE) std::cout << *this <<std::endl;

	int status;
	waitpid(pid, &status, 0);
	return _handleStatus(status); // throw
}

void	CgiHandler::_buildEnvp(Request const& req, std::string const& locRoot) {
	_envp.clear(); // security
	std::map<std::string, std::string> tmp;
	// Essential CGI environment variables
	tmp["REQUEST_METHOD"] = req.getMethod();
	tmp["QUERY_STRING"] = req.getQueryString();
	tmp["CONTENT_TYPE"] = req.getContentType();
	tmp["CONTENT_LENGTH"] = utils::toString(req.getBody().length());
	tmp["SCRIPT_FILENAME"] = _filePath;
	tmp["SCRIPT_NAME"] = utils::trimDomain(req.getPath());
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
	if (utils::getFileExtension(req.getPath()) == ".php") {
		tmp["REDIRECT_STATUS"] = "200";
		tmp["PHP_SELF"] = req.getPath();
	}
	// Build "KEY=VALUE" strings in persistent storage
	for (std::map<std::string, std::string>::const_iterator it = tmp.begin(); it != tmp.end(); ++it)
		_envp.push_back(it->first + "=" + it->second);
}

void	CgiHandler::_buildArgv() {
    _argv.clear(); // security
	_argv.push_back(_executor); // argv[0] = path of the executable (/usr/bin/php-cgi, /usr/bin/python3, etc.)
	if (_extension != ".php")
		_argv.push_back(_filePath); // argv[1] = script (/var/www/index.php, /var/www/website/script.py, etc)
}

pid_t	CgiHandler::_forkAndExec() {
	pid_t pid = fork();
	if (pid == -1)
		throw ExecException("fork() failed");

	if (pid == 0) { // Child
		_redirectIOInChild();
		std::vector<char*> envp = _toCharPtrArray(_envp);
		std::vector<char*> argv = _toCharPtrArray(_argv);
		execve(_executor.c_str(), argv.data(), envp.data());
		_exit(1); // called only if execve failed
	}
	return pid;
}

void	CgiHandler::_communicateWithChild(Request const& req) {
	// close unused pipe ends
	close(_stdinPipe[0]); // body reading
	close(_stdoutPipe[1]); // writing stdout
	close(_stderrPipe[1]); // writing stderr

	// write body in CGI's stdin (if PUT/POST)
	if (req.getMethod() == "POST" || req.getMethod() == "PUT") {
		ssize_t n = write(_stdinPipe[1], req.getBody().c_str(), req.getBody().size());
		(void)n; // mute compiler error
	}
	close(_stdinPipe[1]);

	// Read from pipes
	char buffer[4096];
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

// TODO test this function with request POST of content-length 0 with empty body
Response	CgiHandler::_handleStatus(int status) {
	if (WIFEXITED(status)) {
		if (!_cgiOutput.empty()) {
			Response res(_cgiOutput); // throw Response::RawException (raw response invalid syntax)
			if (!_cgiError.empty()) // warning
				std::cerr << "[WARNING] CGI (" << _executor << "): " << _cgiError << std::endl;
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
void	CgiHandler::_redirectIOInChild() const {
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

std::string const&	CgiHandler::getFilePath() const {
	return _filePath;
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
	os << "- filePath: '" << rhs.getFilePath() << "'\n";
	os << "- executor: '" << rhs.getExecutor() << "'\n";

	std::vector<std::string> envp = rhs.getEnvp();
	os << "- envp: " << envp.size() << "\n";
	for (size_t i = 0; i < envp.size(); ++i)
		os << "  - " << envp[i] << "\n";

	std::vector<std::string> argv = rhs.getArgv();
	os << "- argv: " << argv.size() << "\n";
	for (size_t i = 0; i < argv.size(); ++i)
		os << "  - " << argv[i] << "\n";

	os << "- CGI error: [" << (rhs.getCgiError().empty() ? "empty" : rhs.getCgiError()) << "]\n";
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
