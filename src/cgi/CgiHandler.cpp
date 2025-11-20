#include "cgi/CgiHandler.hpp"

size_t const	CgiHandler::_FILES_THRESHOLD = 10 * 1024 * 1024; // 10MB

CgiHandler::CgiHandler()
{}

CgiHandler::~CgiHandler()
{}

CgiHandler::ExecException::ExecException(std::string const& msg)
: std::runtime_error(msg)
{}

CgiHandler::TimeoutException::TimeoutException(std::string const& msg)
: std::runtime_error(msg)
{}

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
	tmp["PATH_INFO"] = req.getPath().empty() ? "/" : req.getPath(); //!\ full path for the ubuntu_tester to work
	tmp["PATH_TRANSLATED"] = utils::pathsJoin(locRoot, tmp["PATH_INFO"]);
	tmp["QUERY_STRING"] = req.getQueryString();
	tmp["CONTENT_TYPE"] = req.getContentType();
	tmp["CONTENT_LENGTH"] = utils::str(req.getBody().length());
	tmp["DOCUMENT_ROOT"] = locRoot;
	// Server protocol information
	tmp["GATEWAY_INTERFACE"] = "CGI/1.1";
	tmp["SERVER_PROTOCOL"] = req.getVersion();
	tmp["SERVER_SOFTWARE"] = Const::SERVER_SOFTWARE;
	// Additional variables from old subject requirements to pass ubuntu_tester
	tmp["REMOTE_ADDR"] = "0.0.0.0";  //!\ Not implemented (Client IP to be extracted by Network and pass it to Router)
	tmp["REMOTE_IDENT"] = "";
	tmp["AUTH_TYPE"] = ""; //!\ Auth not implemented
	tmp["REMOTE_USER"] = ""; //!\ Auth not implemented
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

std::string const&	CgiHandler::getScriptName() const
{
	return _scriptName;
}

std::string const&	CgiHandler::getExecutor() const
{
	return _executor;
}

std::string const&	CgiHandler::getCgiOutput() const
{
	return _cgiOutput;
}

std::string const&	CgiHandler::getCgiError() const
{
	return _cgiError;
}

std::vector<std::string> const&	CgiHandler::getArgv() const
{
	return _argv;
}

std::vector<std::string> const&	CgiHandler::getEnvp() const
{
	return _envp;
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

// Static utils

void	CgiHandler::_setNonBlocking(int& fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        throw ExecException("fcntl(F_GETFL) failed");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        throw ExecException("fcntl(F_SETFL) failed");
}

void	CgiHandler::_closeFd(int& fd)
{
	if (fd != -1) {
    	close(fd);
    	fd = -1;
	}
}

void	CgiHandler::_initPipe(int pipe[2])
{
	pipe[0] = -1;
	pipe[1] = -1;
}

void	CgiHandler::_closePipe(int pipe[2])
{
	_closeFd(pipe[0]);
	_closeFd(pipe[1]);
}

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
