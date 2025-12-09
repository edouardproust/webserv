#include "http/Response.hpp"
#include "session/SessionManager.hpp"

Response::Response()
: _status(HttpStatus())
, _bodyClearedForHead(false)
, _needsCgiExecution(false)
, _cgiData(NULL)
, _pendingSessionUsername("")
, _expireSession(false)
{
	_initDefaultHeaders();
}

Response::Response(Response const& other)
: _status(other._status)
, _headers(other._headers)
, _setCookieHeaders(other._setCookieHeaders)
, _body(other._body)
, _bodyClearedForHead(other._bodyClearedForHead)
, _servedFilePath(other._servedFilePath)
, _needsCgiExecution(other._needsCgiExecution)
, _cgiData(other._cgiData ? new CgiData(*other._cgiData) : NULL)
, _pendingSessionUsername(other._pendingSessionUsername)
, _expireSession(other._expireSession)
{}

Response& Response::operator=(Response const& other)
{
	if (this != &other) {
		_status = other._status;
		_headers = other._headers;
		_setCookieHeaders = other._setCookieHeaders;
		_body = other._body;
		_bodyClearedForHead = other._bodyClearedForHead;
		_pendingSessionUsername = other._pendingSessionUsername;
		_expireSession = other._expireSession;
		_servedFilePath = other._servedFilePath;
		_needsCgiExecution = other._needsCgiExecution;
		delete _cgiData;
		_cgiData = other._cgiData ? new CgiData(*other._cgiData) : NULL;
	}
	return *this;
}

Response::~Response()
{
	if (_cgiData) {
		delete _cgiData;
		_cgiData = NULL;
	}
}

Response::RawException::RawException(std::string const& msg)
: std::runtime_error(msg)
{}

// PUBLIC METHODS

std::string	Response::stringify(bool onlyHeaders) const
{
	std::stringstream response;
	response << _buildStatusLine();
	response << _buildHeaders();
	response << "\r\n";
	if (!_body.empty() && !onlyHeaders)
		response << _body;
	return response.str();
}

void	Response::clearBody()
{
	_body.clear();
	_bodyClearedForHead = false;
}

void	Response::clearBodyForHead()
{
	bool hadBody = !_body.empty();
	_body.clear();
	_bodyClearedForHead = hadBody;
}

bool	Response::isConnectionClose() const {
	UniqHeaders::const_iterator found = _headers.find("connection");
	if (found == _headers.end())
		return false;
	std::string value = utils::toLowerCase(utils::trim(found->second));
	return value == "close";
}

// CGI

Response	Response::initCgiResponse(RoutingDecision const& rd, std::string const& scriptName, HostPortPair const& listeningOn, std::string const& remoteAddr)
{
	Response resp;
	resp._needsCgiExecution = true;
	resp._cgiData = new CgiData(rd.getRequest(), *rd.getLocation(), scriptName, listeningOn, remoteAddr);
	//Log::dev("debug", "Pending CGI response data:\n" + utils::str(*resp._cgiData));
	return resp;
}

/**
 * Construct a Response from a raw CGI output.
 *
 * Expected format:
 * 	`Status: 200 OK\r\n`
 * 	`Content-Type: text/html\r\n`
 * 	`Other-Header: value\r\n`
 * 	`\r\n`
 * 	`<body>`
 *
 * Throws Response::RawException if headers are malformed or missing Content-Type.
 */
void Response::parseFromCgiOutput(std::string const& cgiOutput, const Request& request)
{
	if (cgiOutput.empty())
		throw RawException("CGI output is empty");

	// Split headers and body
	std::pair<size_t, size_t> const& sep = utils::headersBodySeparatorPos(cgiOutput);
	if (sep.first == std::string::npos)
		throw RawException("malformed CGI output: missing header/body separator");

	std::string headersPart = cgiOutput.substr(0, sep.first);
	std::string bodyPart = cgiOutput.substr(sep.first + sep.second);

	// Parse headers
	parseHeadersFromCgiOutput(headersPart);

	// Set body
	setBodyAndContentLength(bodyPart);

	// Handle session
	_handleSession(request);
}

void Response::parseHeadersFromCgiOutput(std::string const& headersOnly)
{
	if (headersOnly.empty())
		throw RawException("CGI headers empty");

	std::istringstream headerStream(headersOnly);
	std::string line;
	int statusCode = 200; // Default

	while (std::getline(headerStream, line)) {
		// Remove \r if present
		if (!line.empty() && line[line.size() - 1] == '\r')
			line = line.substr(0, line.size() - 1);

		if (line.empty())
			continue;

		size_t colonPos = line.find(':');
		if (colonPos == std::string::npos)
			continue;

		std::string key = line.substr(0, colonPos);
		std::string value = line.substr(colonPos + 1);

		// Trim leading spaces from value
		size_t start = value.find_first_not_of(' ');
		if (start != std::string::npos)
			value = value.substr(start);
		std::string lname = utils::toLowerCase(key);
		// Check if it has session headers
		if (lname == "x-webserv-create-session")
			_pendingSessionUsername = value;
		else if (lname == "x-webserv-destroy-session")
			_expireSession = true;
		// Check if it's the Status header
		else if (lname == "status") {
			std::istringstream statusStream(value);
			statusStream >> statusCode;
		} else
			setHeader(key, value);
	}

	// Vérifier qu'on a bien Content-Type
	if (!_hasHeader("content-type"))
		throw RawException("missing Content-Type header in CGI output");

	// Set status
	setStatus(HttpStatus(statusCode));
}

CgiData*	Response::transferCgiDataOwnership()
{
	CgiData* tmp = _cgiData;
	_cgiData = NULL;  // <- not Response responsability anymore
	return tmp;
}


//SESSION MANAGEMENT

/**
 * Handles session creation and destruction based on CGI headers.
 *
 * Called after parsing CGI output that may contain session control headers:
 * - If `_pendingSessionUsername` is set (from X-Webserv-Create-Session header),
 *   creates a new session and sends Set-Cookie header to client.
 * - If `_expireSession` is set (from X-Webserv-Destroy-Session header),
 *   destroys the current session and expires the client's cookie.
 *
 */
void	Response::_handleSession(const Request& request)
{
	SessionManager& sm = SessionManager::getInstance();

	if (!_pendingSessionUsername.empty()) {
		std::string sessionId = sm.createSession(_pendingSessionUsername);
		std::string maxAge = utils::str(Session::getTimeout());
		std::string options = "HttpOnly; Path=" + SessionManager::COOKIE_PATH + "; Max-Age=" + maxAge;
		addSetCookieHeader(SessionManager::COOKIE_NAME, sessionId, options);
	}
	if (_expireSession) {
		std::string sessionId = request.getCookie(SessionManager::COOKIE_NAME);
		if (!sessionId.empty()) {
			sm.destroySession(sessionId);
			addSetCookieHeader(SessionManager::COOKIE_NAME, "", "HttpOnly; Path=" + SessionManager::COOKIE_PATH + "; Max-Age=0");
		}
	}
}

// PRIVATE METHODS

void	Response::_initDefaultHeaders()
{
	_headers["server"] = Const::SERVER_SOFTWARE;
	_headers["date"] = utils::formatDate(time(0), "%a, %d %b %Y %H:%M:%S GMT");
	_headers["connection"] = "keep-alive";
	_headers["content-length"] = "0";
}

void	Response::_updateContentLength()
{
	std::stringstream lengthStream;
	lengthStream << _body.length();
	_headers["content-length"] = lengthStream.str();
}

void	Response::_manageContentType()
{
	if (!_body.empty() && _headers.find("content-type") == _headers.end())
		_headers["content-type"] = "text/html";
	else if (_body.empty() && !_bodyClearedForHead)
		_headers.erase("content-type");
}

bool	Response::_hasHeader(std::string const& keyLowcase) const
{
	for (UniqHeaders::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
		if (utils::toLowerCase(it->first) == keyLowcase)
			return true;
	}
	return false;
}

std::string Response::_buildStatusLine() const
{
	std::stringstream statusLine;
	statusLine << "HTTP/1.1 " << _status.getCode() << " " << _status.getReason() << "\r\n";
	return statusLine.str();
}

std::string Response::_buildHeaders() const
{
	std::stringstream headerStream;

	headerStream << "Server: " << _headers.find("server")->second << "\r\n";
	headerStream << "Date: " << _headers.find("date")->second << "\r\n";
	if (_headers.find("content-type") != _headers.end())
		headerStream << "Content-Type: " << _headers.find("content-type")->second << "\r\n";
	headerStream << "Content-Length: " << _headers.find("content-length")->second << "\r\n";
	headerStream << "Connection: " << _headers.find("connection")->second << "\r\n";
	for (std::vector<std::string>::const_iterator it = _setCookieHeaders.begin();
		it != _setCookieHeaders.end(); ++it)
			headerStream << "Set-Cookie: " << *it << "\r\n";
	if (_headers.find("location") != _headers.end())
		headerStream << "Location: " << _headers.find("location")->second << "\r\n";
	if (_headers.find("cache-control") != _headers.end())
		headerStream << "Cache-Control: " << _headers.find("cache-control")->second << "\r\n";
	if (_headers.find("content-disposition") != _headers.end())
		headerStream << "Content-Disposition: " << _headers.find("content-disposition")->second << "\r\n";
	if (_headers.find("content-location") != _headers.end())
		headerStream << "Content-Location: " << _headers.find("content-location")->second << "\r\n";
	for (UniqHeaders::const_iterator it = _headers.begin();
		it != _headers.end(); ++it)
		{
			std::string const& key = it->first;
			if (key != "server" && key != "date" && key != "content-type" &&
				key != "content-length" && key != "connection" && key != "location" &&
				key != "cache-control" && key != "content-disposition" && key != "content-location")
					headerStream << key << ": " << it->second << "\r\n";
		}
	return headerStream.str();
}


// SETTERS


void	Response::setStatus(HttpStatus const& status)
{
	_status = status;
	if (status.getSlug() == "no_content")
		setBodyAndContentLength("");
}

/**
 * @param name Not case-sensitive
 */
void	Response::setHeader(std::string const& name, std::string const& value)
{
	std::string normalizedName = utils::toLowerCase(name);

	if (normalizedName == "connection")
	{
		std::string normalizedValue = utils::toLowerCase(value);
		if (normalizedValue == "keep-alive" || normalizedValue == "close")
			_headers[normalizedName] = normalizedValue;
		else
			_headers[normalizedName] = "keep-alive";
		return ;
	}
	_headers[normalizedName] = value;
}

void	Response::setBodyAndContentLength(std::string const& body)
{
	_body = body;
	_updateContentLength();
	_manageContentType();
}

void	Response::setServedFilePath(std::string const& absoluteFilePath)
{
	_servedFilePath = absoluteFilePath;
}

void	Response::setConnectionFromRequest(Request const& request)
{
	if (request.isConnectionClose())
		setHeader("Connection", "close");
	else
		setHeader("Connection", "keep-alive");
}

void	Response::addSetCookieHeader(const std::string& name, const std::string& value, const std::string& options)
{
	std::string setCookieHeader = name + "=" + value;

	if (!options.empty())
		setCookieHeader += "; " + options;
	_setCookieHeaders.push_back(setCookieHeader);
}

// GETTERS

HttpStatus const&	Response::getStatus() const
{
	return _status;
}

UniqHeaders const&	Response::getHeaders() const
{
	return _headers;
}

const std::vector<std::string>& Response::getSetCookieHeaders() const
{
	return _setCookieHeaders;
}

std::string const&	Response::getBody() const
{
	return _body;
}

std::string const&	Response::getServedFilePath() const
{
	return _servedFilePath;
}

bool	Response::needsCgiExecution() const
{
	return _needsCgiExecution;
}

CgiData const*	Response::getCgiData() const
{
	return _cgiData;
}

const std::string&	Response::getPendingSessionUsername() const
{
	return _pendingSessionUsername;
}

// PRINT

std::ostream& operator<<(std::ostream& os, Response const& response)
{
	os << "- Status: " << PrintableString(response.getStatus().toStr()) << "\n";
	os << "- Served file path: " << PrintableString(response.getServedFilePath()) << "\n";
	os << "- Headers: " << response.getHeaders().size() << "\n";
	UniqHeaders const& headers = response.getHeaders();
	for (UniqHeaders::const_iterator it = headers.begin();
		it != headers.end(); ++it)
			os << "  - " << PrintableString(it->first) << ": " << PrintableString(it->second) << "\n";
	os << "- Body: " << (response.getBody().empty() ? "no" : "yes") << "\n";
	os << "- Body Length: " << response.getBody().length() << "\n";
	os << "- Raw HTTP Response Preview:\n" << PrintableString(Log::excerpt(Log::EXCERPT_SIZE, response.stringify())) << "\n";
	return os;
}