#include "http/Response.hpp"

Response::Response()
: _status(HttpStatus())
, _headers()
, _body()
, _bodyClearedForHead(false)
, _servedFilePath()
, _needsCgiExecution(false)
, _cgiParams(NULL)
, _cgiRequest(NULL)
{
	_initDefaultHeaders();
}

/**
 * May throw a RawException.
 */
Response::Response(std::string const& rawResponse)
: _status(HttpStatus())
, _headers()
, _body()
, _bodyClearedForHead(false)
, _servedFilePath()
, _needsCgiExecution(false)
, _cgiParams(NULL)
, _cgiRequest(NULL)
{
	_initDefaultHeaders();
	_parseRawResponse(rawResponse); // throw
}

Response::Response(Response const& other)
: _status(other._status)
, _headers(other._headers)
, _body(other._body)
, _bodyClearedForHead(other._bodyClearedForHead)
, _servedFilePath(other._servedFilePath)
, _needsCgiExecution(other._needsCgiExecution)
, _cgiParams(other._cgiParams ? new CgiParams(*other._cgiParams) : NULL) // Deep copy
, _cgiRequest(other._cgiRequest) // Pointer copy
{}

Response& Response::operator=(Response const& other)
{
	if (this != &other)
	{
		_status = other._status;
		_headers = other._headers;
		_body = other._body;
		_bodyClearedForHead = other._bodyClearedForHead;
		_servedFilePath = other._servedFilePath;
		_needsCgiExecution = other._needsCgiExecution;
		delete _cgiParams;
		_cgiParams = other._cgiParams ? new CgiParams(*other._cgiParams) : NULL; // Deep copy
		_cgiRequest = other._cgiRequest; // Pointer copy
	}
	return *this;
}

Response::~Response()
{
	delete _cgiParams;
	// no delete for _cgiRequest (non-owning)
}

Response::RawException::RawException(std::string const& msg)
: std::runtime_error(msg)
{}

void	Response::_initDefaultHeaders()
{
	_headers["server"] = Const::SERVER_SOFTWARE;
	_headers["date"] = utils::formatDate(time(0), "%a, %d %b %Y %H:%M:%S GMT");
	_headers["connection"] = "keep-alive";
	_headers["content-length"] = "0";
}

std::string	Response::stringify() const
{
	std::stringstream response;

	response << _buildStatusLine();
	response << _buildHeaders();
	response << "\r\n";
	if (!_body.empty())
		response << _body;
	return response.str();
}

void	Response::setStatus(HttpStatus const& status)
{
	_status = status;
	if (status.getCode() == 204)
		setBody("");
}

/**
 * Set header "Content-Type".
 * It is a wrapper of `setHeader` method that prevents mispelling.
 */
void	Response::setContentType(std::string const& value)
{
	setHeader("Content-Type", value);
}

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

void	Response::setBody(std::string const& body)
{
	_body = body;
	_updateContentLength();
	_manageContentType();
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

void	Response::setServedFilePath(std::string const& filePath)
{
	_servedFilePath = filePath;
}

void	Response::setConnectionFromRequest(Request const& request)
{
	UniqHeaders const& headers = request.getUniqHeaders();
	UniqHeaders::const_iterator it = headers.find("connection");
	if (it != headers.end())
		setHeader("Connection", it->second);
	else
		setHeader("Connection", "keep-alive");
}

bool	Response::isConnectionClose() const {
	UniqHeaders::const_iterator found = _headers.find("connection");
	if (found == _headers.end())
		return false;
	std::string value = utils::toLowerCase(utils::trim(found->second));
	return value == "close";
}

void Response::markForCgiExecution(CgiParams const& params, Request const& req) {
	_needsCgiExecution = true;
	_cgiParams = new CgiParams(params);
	_cgiRequest = &req;  // Reference only (can have a big body)
}

HttpStatus const&	Response::getStatus() const
{
	return _status;
}

UniqHeaders const& Response::getHeaders() const
{
	return _headers;
}

std::string const& Response::getBody() const
{
	return _body;
}

bool Response::needsCgiExecution() const {
    return _needsCgiExecution;
}

CgiParams const& Response::getCgiParams() const {
	if (!_cgiParams)
		throw std::runtime_error("No CGI params in response");
	return *_cgiParams;
}

Request const& Response::getCgiRequest() const {
	if (!_cgiRequest)
		throw std::runtime_error("No CGI request in response");
	return *_cgiRequest;
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
	if (_headers.find("location") != _headers.end())
		headerStream << "Location: " << _headers.find("location")->second << "\r\n";
	if (_headers.find("cache-control") != _headers.end())
		headerStream << "Cache-Control: " << _headers.find("cache-control")->second << "\r\n";
	if (_headers.find("content-disposition") != _headers.end())
		headerStream << "Content-Disposition: " << _headers.find("content-disposition")->second << "\r\n";
	for (UniqHeaders::const_iterator it = _headers.begin();
		it != _headers.end(); ++it)
		{
			std::string const& key = it->first;
			if (key != "server" && key != "date" && key != "content-type" &&
				key != "content-length" && key != "connection" &&
				key != "location" && key != "cache-control" && key != "content-disposition")
					headerStream << key << ": " << it->second << "\r\n";
		}
	return headerStream.str();
}

// static utils

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
void	Response::_parseRawResponse(std::string const& rawResponse)
{
	if (rawResponse.empty())
		throw RawException("raw response is empty");

	// Split headers and body
	size_t headerEnd = rawResponse.find("\r\n\r\n");
	size_t skip = 4;
	if (headerEnd == std::string::npos) {
		headerEnd = rawResponse.find("\n\n");
		skip = 2;
	}
	if (headerEnd == std::string::npos)
		throw RawException("malformed raw response: missing header/body separator");
	std::string headersPart = rawResponse.substr(0, headerEnd);
	std::string bodyPart = rawResponse.substr(headerEnd + skip);

	// Set Response attributes
	int statusCode = _setHeaders(headersPart);
	if (!_hasHeader("content-type"))
		throw RawException("missing Content-Type header");
	setStatus(HttpStatus(statusCode));
	setBody(bodyPart);
}

int	Response::_setHeaders(std::string const& headersPart) {
	std::istringstream headersStream(headersPart);
	std::string line;
	int statusCode = HttpStatus("ok").getCode(); // default if no Status header found

	while (std::getline(headersStream, line)) {
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (line.empty())
			continue;

		size_t colonPos = line.find(':');
		if (colonPos == std::string::npos)
			throw RawException("invalid header line: " + line);

		std::string name = line.substr(0, colonPos);
		std::string value = line.substr(colonPos + 1);
		value = utils::trim(value);
		std::string lname = utils::toLowerCase(name);
		std::istringstream iss(value);
		if (lname == "status") { // eg. "Status: 404 Not Found"
			iss >> statusCode; // if fail: keep default value 200
			continue;
		}
		setHeader(name, value);
	}
	return statusCode;
}

std::ostream& operator<<(std::ostream& os, Response const& response)
{
	os << "- Status: " << PrintableString(response.getStatus().toStr()) << "\n";
	os << "- Served file path: " << PrintableString(response.getServedFilePath()) << "\n";
	os << "- Needs CGI Execution: " << (response.needsCgiExecution() ? "Yes" : "No") << "\n";
	os << "- Headers: " << response.getHeaders().size() << "\n";
	UniqHeaders const& headers = response.getHeaders();
	for (UniqHeaders::const_iterator it = headers.begin(); it != headers.end(); ++it) {
		os << "  - " << PrintableString(it->first) << ": " << PrintableString(it->second) << "\n";
	}
	if (response.needsCgiExecution()) {
		os << response.getCgiParams();
	}
	os << "- Raw response:\n" << PrintableString(Log::excerpt(Log::EXCERPT_CHARS, response.stringify())) << "\n";
	return os;
}