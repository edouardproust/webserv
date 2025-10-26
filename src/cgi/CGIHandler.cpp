#include "cgi/CGIHandler.hpp"
#include <sys/wait.h>

CGIHandler::CGIHandler() {}

Response	CGIHandler::handleRequest(Request const& req, std::string const& scriptPath, std::string const& executor)
{
	std::vector<char*> envp = _buildEnvp(req, scriptPath);
	std::vector<char*> argv = _buildArgv(executor, scriptPath);

	// Pipes
	int stdinPipe[2];
	int stdoutPipe[2];
	if (pipe(stdinPipe) == -1 || pipe(stdoutPipe) == -1)
		throw std::runtime_error("pipe() failed"); // TODO
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
		execve(executor.c_str(), argv.data(), envp.data());
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
		std::string cgiOutput;
		char buffer[4096];
		ssize_t n;
		while ((n = read(stdoutPipe[0], buffer, sizeof(buffer))) > 0)
			cgiOutput.append(buffer, n);
		close(stdoutPipe[0]);
		std::cout << "[DEBUG] cgiOutput: " << cgiOutput << std::endl; // DEBUG
		int status;
		waitpid(pid, &status, 0);
		return Response(); // DEBUG Return an empty Response for now to allow compilation
	}
}

std::vector<char*>	CGIHandler::_buildEnvp(Request const& req, std::string const& scriptPath) {
	std::vector<std::string> tmp;
	// Essential CGI env. variables
	tmp.push_back("REQUEST_METHOD=" + req.getMethod());
	tmp.push_back("QUERY_STRING=" + req.getQueryString());
	tmp.push_back("CONTENT_TYPE=" + req.getContentType());
	tmp.push_back("CONTENT_LENGTH=" + utils::toString(req.getBody().length()));
	tmp.push_back("SCRIPT_FILENAME=" + scriptPath);
	tmp.push_back("SCRIPT_NAME=" + utils::trimDomain(req.getPath()));
	// Other important env. variables
	tmp.push_back("GATEWAY_INTERFACE=CGI/1.1");
	tmp.push_back("SERVER_PROTOCOL=HTTP/" + req.getVersion());
	tmp.push_back("SERVER_SOFTWARE=" + SERVER_SOFTWARE);
	// Conversion of HTTP headers into env. variables (CGI standard)
	Headers headers = req.getHeaders();
	for (Headers::const_iterator it = headers.begin(); it != headers.end(); ++it)
        tmp.push_back(_headerToEnvVar(it->first) + "=" + it->second);
	// Convert envVars to char* for execve
	std::vector<char*> envp ;
	for (size_t i = 0; i < tmp.size(); ++i)
		envp.push_back(const_cast<char*>(tmp[i].c_str()));
	envp.push_back(NULL);
	return envp;
}

std::vector<char*>	CGIHandler::_buildArgv(std::string const& executor, std::string const& scriptPath) {
	std::vector<char*> argv;
	argv.push_back(const_cast<char*>(executor.c_str())); // eg. /usr/bin/php-cgi
	argv.push_back(const_cast<char*>(scriptPath.c_str())); // eg. /var/www/html/hello.php
	argv.push_back(NULL);
	return argv;
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

void	CGIHandler::_writeBodyToStdin(std::string const& body) {
	std::cout << "[TODO: Writing body to CGI stdin]" << body << std::endl;
	(void)body; // to silence unused parameter warning
}

void	CGIHandler::_executeScript(std::string const& executor, std::string const& scriptPath) {
	std::cout << "[TODO: Executing CGI script]" << std::endl;
	std::cout << "Executor: " << executor << std::endl;
	std::cout << "Script Path: " << scriptPath << std::endl;
}
