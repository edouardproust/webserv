#include "http/Response.hpp"

Response::Response()
: _status(HttpStatus())
{
	_initDefaultHeaders();
}

/**
 * May throw a RawException.
 */
Response::Response(std::string const& rawResponse)
: _status(HttpStatus())
{
	_initDefaultHeaders();
	_parseRawResponse(rawResponse); // throw
}

Response::Response(Response const& other)
: _status(other._status)
, _headers(other._headers)
, _body(other._body)
{}

Response& Response::operator=(Response const& other)
{
	if (this != &other)
	{
		_status = other._status;
		_headers = other._headers;
		_body = other._body;
	}
	return *this;
}

Response::~Response()
{}

Response::RawException::RawException(std::string const& msg)
: std::runtime_error(msg)
{}

void	Response::_initDefaultHeaders()
{
	_headers["server"] = std::make_pair("Server", Const::SERVER_SOFTWARE);
	_headers["date"] = std::make_pair("Date", utils::formatDate(time(0), "%a, %d %b %Y %H:%M:%S GMT"));
	_headers["connection"] = std::make_pair("Connexion", "keep-alive");
	_headers["content-length"] = std::make_pair("Content-Length", "0");
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

HttpStatus const&	Response::getStatus() const
{
	return _status;
}

std::map<std::string, Response::Header> const& Response::getHeaders() const
{
	return _headers;
}

std::string const& Response::getBody() const
{
	return _body;
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
	std::string slug = utils::toLowerCase(name);
	std::string v = value;

	if (slug == "connection" && utils::toLowerCase(v) != "close")
		v = "keep-alive";
	_headers[slug] = std::make_pair(name, v);
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
	_manageContentType();
}

void	Response::setConnectionFromRequest(Request const& request)
{
	const std::map<std::string, std::string>& headers = request.getHeaders();
	if (headers.find("connection") != headers.end())
		setHeader("Connection", headers.find("connection")->second);
	else
		setHeader("Connection", "keep-alive");
}

void	Response::_updateContentLength()
{
	std::stringstream lengthStream;
	lengthStream << _body.length();
	_headers["content-length"].second = lengthStream.str();
}

void	Response::_manageContentType()
{
	if (_body.empty())
		_headers.erase("content-type");
	else if (_headers.find("content-type") == _headers.end() && !_body.empty())
		_headers["content-type"].second = "text/html";
}

bool	Response::_hasHeader(std::string const& keyLowcase) const
{
	for (std::map<std::string, Header>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
		if (utils::toLowerCase(it->first) == keyLowcase)
			return true;
	}
	return false;
}

std::string Response::_buildStatusLine() const
{
	std::stringstream statusLine;

	statusLine << "HTTP/1.1 " << _status.toStr() << "\r\n";
	return statusLine.str();
}

std::string Response::_buildHeaders() const
{
	std::string headersStr;
	for (std::map<std::string, Header>::const_iterator it = _headers.begin();
     it != _headers.end(); ++it)
    	headersStr += it->second.first + ": " + it->second.second + "\r\n";
	return headersStr;
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

bool	Response::isConnectionClose() const {
    std::map<std::string, Response::Header>::const_iterator found = _headers.find("connection");
    return (found != _headers.end() && found->second.second == utils::toLowerCase("close"));
}

std::ostream& operator<<(std::ostream& os, Response const& response)
{
	os << "- Status: " << PrintableString(response.getStatus().toStr()) << "\n";
	os << "- Headers: " << response.getHeaders().size() << "\n";
	const std::map<std::string, Response::Header>& headers = response.getHeaders();
	for (std::map<std::string, Response::Header>::const_iterator it = headers.begin();
		it != headers.end(); ++it)
			os << "  - " << PrintableString(it->second.first) << ": " << PrintableString(it->second.second) << "\n";
	os << "- Body: " << (response.getBody().empty() ? "no" : "yes") << "\n";
	os << "- Body Length: " << response.getBody().length() << "\n";
	os << "- Raw HTTP Response Preview:\n" << PrintableString(Log::excerpt(Log::EXCERPT_CHARS, response.stringify())) << "\n";
	return os;
}
