#include "cgi/CgiData.hpp"
#include "network/Client.hpp"

CgiData::CgiData(CgiData const& other)
: _scriptName(other._scriptName)
, _extension(other._extension)
, _executor(other._executor)
, _request(other._request)
, _errorPages(other._errorPages)
, _remoteAddr(other._remoteAddr)
, _envStorage(other._envStorage)
, _argStorage(other._argStorage)
{
	_setEnvp();
	_setArgv();
}

CgiData::CgiData(Request const& req, LocationBlock const& loc, std::string const& scriptName, HostPortPair const& listeningOn, std::string const& remoteAddr)
: _scriptName(scriptName)
, _extension(utils::getFileExtension(_scriptName))
, _executor(loc.getCgiExecutor(_extension))
, _request(req)
, _errorPages(loc.getErrorPages())
, _remoteAddr(remoteAddr)
{
	if (_extension.empty() || _executor.empty()) {
		Log::prod("error", "CGI params: failed to initalise");
		return; // isValid = false
	}
	_setEnvStorage(req, loc.getRoot(), listeningOn);
	_setEnvp();
    _setArgStorage();
	_setArgv();
}

CgiData::~CgiData()
{}

/**
 * Builds the environment variables for the CGI script from the Request and LocationBlock.
 *
 * Stores them in _envp.
 */
void	CgiData::_setEnvStorage(Request const& req, std::string const& locRoot, HostPortPair const& listeningOn)
{
	_envStorage.clear(); // security
	UniqHeaders tmp;
	// Essential CGI environment variables
	tmp["REQUEST_METHOD"] = req.getMethod();
	tmp["SCRIPT_FILENAME"] = _scriptName; // absolute path
	tmp["SCRIPT_NAME"] = req.getScriptName(); // relative path
	if (UBUNTU_TESTER)
		tmp["PATH_INFO"] = req.getPath().empty() ? "/" : req.getPath(); //!\ full path for the ubuntu_tester to work
	else
		tmp["PATH_INFO"] = req.getPathInfo();
	tmp["PATH_TRANSLATED"] = utils::pathsJoin(locRoot, tmp["PATH_INFO"]);
	tmp["QUERY_STRING"] = req.getQueryString();
	tmp["CONTENT_TYPE"] = req.getContentType();
	tmp["CONTENT_LENGTH"] = utils::str(getRequest().getBody().size());
	tmp["DOCUMENT_ROOT"] = locRoot;
	// Server protocol information
	tmp["GATEWAY_INTERFACE"] = "CGI/1.1";
	tmp["SERVER_PROTOCOL"] = req.getVersion();
	tmp["SERVER_SOFTWARE"] = Const::SERVER_SOFTWARE;
	tmp["REMOTE_ADDR"] = _remoteAddr;
	tmp["REMOTE_HOST"] = _remoteAddr; // Same as REMOTE_ADDR/default
	tmp["REMOTE_IDENT"] = ""; // Not implemented
	tmp["AUTH_TYPE"] = ""; //!\ Auth not implemented
	tmp["REMOTE_USER"] = ""; //!\ Auth not implemented
	tmp["REQUEST_URI"] = req.getUri();
	tmp["SERVER_NAME"] = listeningOn.getHost();
	tmp["SERVER_PORT"] = utils::str(listeningOn.getPort());
	// HTTP headers (prefixed with HTTP_)
	UniqHeaders const& headers = req.getUniqHeaders();
	for (UniqHeaders::const_iterator it = headers.begin(); it != headers.end(); ++it)
		tmp[_headerToEnvVar(it->first)] = it->second;
	// PHP specific variables
	if (_extension == ".php") {
		tmp["REDIRECT_STATUS"] = HttpStatus("ok").getCodeStr();
		tmp["PHP_SELF"] = req.getPath();
	}
	// Build "KEY=VALUE" strings in persistent storage
	for (UniqHeaders::const_iterator it = tmp.begin(); it != tmp.end(); ++it)
		_envStorage.push_back(it->first + "=" + it->second);
}

/**
 * Builds the argument vector for execve, including the executor and script path.
 *
 * Stores them in _argv.
 */
void	CgiData::_setArgStorage()
{
    _argStorage.clear();
	_argStorage.push_back(_executor); // argv[0] = path of the executable (/usr/bin/php-cgi, /usr/bin/python3, etc.)
	_argStorage.push_back(_scriptName); // argv[1] = script (/var/www/index.php, /var/www/website/script.py, etc)
}

/**
 * Prepare array for execve, containing environement variables for CGI executable
 */
void	CgiData::_setEnvp()
{
	_envp.clear();
	for (size_t i = 0; i < _envStorage.size(); ++i) {
		_envp.push_back(const_cast<char*>(_envStorage[i].c_str()));
	}
	_envp.push_back(NULL);
}

void	CgiData::_setArgv()
{
	_argv.clear();
	for (size_t i = 0; i < _argStorage.size(); ++i) {
		_argv.push_back(const_cast<char*>(_argStorage[i].c_str()));
	}
	_argv.push_back(NULL);
}

std::string const&	CgiData::getScriptName() const
{
	return _scriptName;
}

std::string const&	CgiData::getExtension() const
{
	return _extension;
}

std::string const&	CgiData::getExecutor() const
{
	return _executor;
}

Request const&	CgiData::getRequest() const
{
	return _request;
}

ErrorPages const&	CgiData::getErrorPages() const
{
	return _errorPages;
}

bool	CgiData::isValid() const
{
	return _isValid;
}

std::vector<std::string> const&	CgiData::getEnvStorage() const
{
	return _envStorage;
}

std::vector<char*> const&	CgiData::getEnvp() const
{
	return _envp;
}

std::vector<std::string> const&	CgiData::getArgStorage() const
{
	return _argStorage;
}

std::vector<char*> const&	CgiData::getArgv() const
{
	return _argv;
}

// UTILS (static)

/**
 * Converts a header name (e.g., "Content-Type") into a CGI environment variable format
 * (e.g., "HTTP_CONTENT_TYPE").
 */
std::string	CgiData::_headerToEnvVar(std::string const& headerName)
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

// PRINT

std::ostream&	operator<<(std::ostream& os, CgiData const& rhs)
{
	os << "- script name: " << PrintableString(rhs.getScriptName()) << "\n";
	os << "- executor: " << PrintableString(rhs.getExecutor()) << "\n";
	os << "- input data size: " << rhs.getRequest().getBody().size() << "\n";

	std::vector<std::string> argv = rhs.getArgStorage();
	os << "- argv: " << argv.size() << "\n";
	for (size_t i = 0; i < argv.size(); ++i)
		os << "  - " << PrintableString(argv[i]) << "\n";

	std::vector<std::string> envp = rhs.getEnvStorage();
	os << "- envp: " << envp.size() << "\n";
	for (size_t i = 0; i < envp.size(); ++i)
		os << "  - " << PrintableString(envp[i]) << "\n";

	return os;
}
