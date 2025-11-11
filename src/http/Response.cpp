#include "http/Response.hpp"

Response::Response() : _status(HttpStatus("ok"))
{
	_initDefaultHeaders();
}

/**
 * May throw a RawException.
 */
Response::Response(std::string const& rawResponse): _status(HttpStatus("ok")) {
	_initDefaultHeaders();
	_parseRawResponse(rawResponse); // throw
}

Response::Response(const Response& other)
	: _status(other._status),
	  _headers(other._headers),
	  _body(other._body) {}

Response& Response::operator=(const Response& other)
{
	if (this != &other)
	{
		_status = other._status;
		_headers = other._headers;
		_body = other._body;
	}
	return *this;
}

Response::~Response() {}

Response::RawException::RawException(std::string const& msg)
: std::runtime_error(msg) {}

void	Response::_initDefaultHeaders() {
	_headers["server"] = SERVER_SOFTWARE;
	_headers["date"] = _getCurrentDate();
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

const HttpStatus&	Response::getStatus() const
{
	return _status;
}

const std::map<std::string, std::string>& Response::getHeaders() const
{
	return _headers;
}

const std::string& Response::getBody() const
{
	return _body;
}

void	Response::setStatus(const HttpStatus& status)
{
	_status = status;
	if (status.getCode() == 204)
		setBody("");
}

/**
 * Set header "Content-Type".
 * It is a wrapper of `setHeader` method that prevents mispelling.
 */
void	Response::setContentType(const std::string& value) {
	setHeader("Content-Type", value);
}

void	Response::setHeader(const std::string& name, const std::string& value)
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

void	Response::setBody(const std::string& body)
{
	_body = body;
	_updateContentLength();
	_manageContentType();
}

void	Response::setConnectionFromRequest(const Request& request)
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
	_headers["content-length"] = lengthStream.str();
}

void	Response::_manageContentType()
{
	if (_body.empty())
		_headers.erase("content-type");
	else if (_headers.find("content-type") == _headers.end() && !_body.empty())
		_headers["content-type"] = "text/html";
}

std::string Response::_getCurrentDate() const
{
	time_t now = time(0);
	struct tm* timeinfo = gmtime(&now);
	char buffer[80];
	strftime(buffer, 80, "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
	return buffer;
}

bool	Response::_hasHeader(const std::string& keyLowcase) const {
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
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
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
		it != _headers.end(); ++it)
		{
			const std::string& key = it->first;
			if (key != "server" && key != "date" && key != "content-type" &&
				key != "content-length" && key != "connection" &&
				key != "location" && key != "cache-control")
					headerStream << key << ": " << it->second << "\r\n";
		}
	return headerStream.str();
}

std::ostream& operator<<(std::ostream& os, const Response& response)
{
	os << "Response:\n";
	os << "- Status: " << response.getStatus().getCode() << " " << response.getStatus().getReason() << "\n";
	os << "- Headers: " << response.getHeaders().size() << "\n";
	const std::map<std::string, std::string>& headers = response.getHeaders();
	for (std::map<std::string, std::string>::const_iterator it = headers.begin();
		it != headers.end(); ++it)
			os << "  - " << it->first << ": " << it->second << "\n";
	os << "- Body: " << (response.getBody().empty() ? "no" : "yes") << "\n";
	os << "- Body Length: " << response.getBody().length() << "\n";
	os << "- Raw HTTP Response Preview:\n[" << utils::excerpt(EXCERPT_LENGTH, response.stringify()) << "]\n";
	return os;
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
void	Response::_parseRawResponse(const std::string& rawResponse)
{
	if (rawResponse.empty())
		throw RawException("raw response is empty");

	// Split headers and body
	size_t headerEnd = rawResponse.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
		headerEnd = rawResponse.find("\n\n");
	if (headerEnd == std::string::npos)
		throw RawException("malformed raw response: missing header/body separator");
	std::string headersPart = rawResponse.substr(0, headerEnd);
	std::string bodyPart = rawResponse.substr(headerEnd + 4);

	// Set Response attributes
	int statusCode = _setHeaders(headersPart);
	if (!_hasHeader("content-type"))
		throw RawException("missing Content-Type header");
	setStatus(HttpStatus(statusCode));
	setBody(bodyPart);
}

int	Response::_setHeaders(const std::string& headersPart) {
	std::istringstream headersStream(headersPart);
	std::string line;
	int statusCode = 200; // default if no Status header found

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

