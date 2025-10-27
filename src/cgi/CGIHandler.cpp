#include "cgi/CGIHandler.hpp"
#include <sys/wait.h>

CGIHandler::CGIHandler() {}

CGIHandler::~CGIHandler() {}

/**
 * Throw a runtime exception if fork() or pipe() fail.
 */
Response	CGIHandler::handleRequest(Request const& req, LocationBlock const* loc, std::string const& filePath) {
	_filePath = filePath;
	_extension = utils::getFileExtension(_filePath);
	_executor = loc->getCgiExecutor(_extension);

	// Pipes
	int stdinPipe[2];
	int stdoutPipe[2];
	if (pipe(stdinPipe) == -1 || pipe(stdoutPipe) == -1)
		throw std::runtime_error("pipe() failed"); // TODO

	// Variables for execve
	_buildEnvp(req, filePath, loc->getRoot());
	_buildArgv(_executor, _filePath);
	std::vector<char*> envp = _toCharPtrArray(_envp);
	std::vector<char*> argv = _toCharPtrArray(_argv);

	// Fork
	pid_t pid = fork();
	if (pid == -1)
		throw std::runtime_error("fork() failed"); // TODO
	// Handle process
	if (pid == 0) { // Child process
		// redirect stdin
		close(stdinPipe[1]); // close writing
		dup2(stdinPipe[0], STDIN_FILENO);
		close(stdinPipe[0]);
		// redirect stdout
		close(stdoutPipe[0]); // close reading
		dup2(stdoutPipe[1], STDOUT_FILENO);
		close(stdoutPipe[1]);
		// execute CGI script
		execve(_executor.c_str(), argv.data(), envp.data());
		//perror("execve"); // TODO
		_exit(1);
	} else { // Parent process
		close(stdinPipe[0]); // close body reading
		close(stdoutPipe[1]); // close writing to CGI's stdout
		// write body in CGI's stdin (if PUT/POST)
		if (req.getMethod() == "POST" || req.getMethod() == "PUT")
			write(stdinPipe[1], req.getBody().c_str(), req.getBody().size());
		close(stdinPipe[1]);
		// reading CGI's output
		char buffer[4096];
		ssize_t n;
		while ((n = read(stdoutPipe[0], buffer, sizeof(buffer))) > 0)
			_cgiOutput.append(buffer, n);
		close(stdoutPipe[0]);
		int status;
		waitpid(pid, &status, 0);

		// TODO Parse Response!
		return Response();
	}
	}

	void	CGIHandler::_buildEnvp(Request const& req, std::string const& filePath, std::string const& locRoot) {
	_envp.clear(); // security
	std::map<std::string, std::string> tmp;
	// Essential CGI environment variables
	tmp["REQUEST_METHOD"] = req.getMethod();
	tmp["QUERY_STRING"] = req.getQueryString();
	tmp["CONTENT_TYPE"] = req.getContentType();
	tmp["CONTENT_LENGTH"] = utils::toString(req.getBody().length());
	tmp["SCRIPT_FILENAME"] = filePath;
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

void	CGIHandler::_buildArgv(std::string const& executor, std::string const& filePath) {
    _argv.clear(); // security
	_argv.push_back(executor); // argv[0] = path of the executable (/usr/bin/php-cgi, /usr/bin/python3, etc.)
	_argv.push_back(filePath); // argv[1] = script (/var/www/index.php, /var/www/website/script.py, etc)
}

std::string	CGIHandler::_headerToEnvVar(const std::string& headerName) {
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

std::vector<char*>	CGIHandler::_toCharPtrArray(const std::vector<std::string>& src) {
	std::vector<char*> result;
	result.reserve(src.size() + 1);
	for (size_t i = 0; i < src.size(); ++i)
		result.push_back(const_cast<char*>(src[i].c_str()));
	result.push_back(NULL);
	return result;
}

std::string const&	CGIHandler::getFilePath() const {
	return _filePath;
}

std::string const&	CGIHandler::getExecutor() const {
	return _executor;
}

std::vector<std::string> const&	CGIHandler::getEnvp() const {
	return _envp;
}

std::vector<std::string> const&	CGIHandler::getArgv() const {
	return _argv;
}

std::string const&	CGIHandler::getCgiOutput() const {
	return _cgiOutput;
}

std::ostream&	operator<<(std::ostream& os, CGIHandler const& rhs) {
	os << "CGIHandler:\n"
		<< "- filePath: '" << rhs.getFilePath() << "'\n"
		<< "- executor: '" << rhs.getExecutor() << "'\n";

		std::vector<std::string> envp = rhs.getEnvp();
		os << "- envp: " << envp.size() << "\n";
		for (size_t i = 0; i < envp.size(); ++i)
			os << "  - " << envp[i] << "\n";

		std::vector<std::string> argv = rhs.getArgv();
		os << "- argv: " << argv.size() << "\n";
		for (size_t i = 0; i < argv.size(); ++i)
			os << "  - " << argv[i] << "\n";

	os << "- cgiOutput:\n------\n" << rhs.getCgiOutput() << "------\n";

	return os;
}

