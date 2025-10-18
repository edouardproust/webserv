#include "http/Response.hpp"
#include "utils/utils.hpp"
#include <ctime>

Response::Response() : _statusCode(200), _reasonPhrase("OK")
{
	_headers["server"] = "webserv/1.0";
	_headers["date"] = _getCurrentDate();
	_headers["connection"] = "keep-alive";
}

Response::Response(const Response& other)
	: _statusCode(other._statusCode),
	  _reasonPhrase(other._reasonPhrase),
	  _headers(other._headers),
	  _body(other._body) {}

Response& Response::operator=(const Response& other)
{
	if (this != &other)
	{
		_statusCode = other._statusCode;
		_reasonPhrase = other._reasonPhrase;
		_headers = other._headers;
		_body = other._body;
	}
	return *this;
}

Response::~Response() {}

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

int	Response::getStatusCode() const
{
	return _statusCode;
}

const std::string& Response::getReasonPhrase() const
{
	return _reasonPhrase;
}

const std::map<std::string, std::string>& Response::getHeaders() const
{
	return _headers;
}

const std::string& Response::getBody() const
{
	return _body;
}

void	Response::setStatusCode(int statusCode)
{
	_statusCode = statusCode;
	_reasonPhrase = _getReasonPhrase(statusCode);
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
	std::stringstream lengthStream;
	lengthStream << body.length();
	_headers["content-length"] = lengthStream.str();
	if (_headers.find("content-type") == _headers.end() && !body.empty())
		_headers["content-type"] = "text/html";
	if (body.empty())
		_headers.erase("content-type");
}

void	Response::setError(int statusCode)
{
	setStatusCode(statusCode);
	setHeader("Content-Type", "text/html");
	std::string errorBody = _generateErrorPage();
	setBody(errorBody);
}

std::string Response::_getReasonPhrase(int statusCode) const
{
	switch(statusCode)
	{
		//Success
		case 200:
			return "OK";
		case 201:
			return "Created";
		case 204:
			return "No Content";
		 // Redirection (for CGI/location blocks)
		case 301:
			return "Moved Permanently";
		case 302:
			return "Found";
		// Client Errors
		case 400:
			return "Bad Request";
		case 403:
			return "Forbidden";
		case 404:
			return "Not Found";
		case 405:
			return "Method Not Allowed";
		case 411:
			return "Length Required";
		case 413:
			return "Content Too Large";
		// Server Errors
		case 500:
			return "Internal Server Error";
		case 501:
			return "Not Implemented";
		case 502:
			return "Bad Gateway";
		case 505:
			return "HTTP Version Not Supported";
		//more status codes to be added accordingly!!
		default:
			return "Unknown";
	}	
}

std::string Response::_getCurrentDate() const
{
	time_t now = time(0);
	struct tm* timeinfo = gmtime(&now);
	char buffer[80];
	strftime(buffer, 80, "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
	return buffer;
}

std::string Response::_generateErrorPage() const
{
	std::stringstream html;
	std::string reasonPhrase = _getReasonPhrase(_statusCode);

	html << "<html>\n"
		 << "  <head>\n"
		 << "    <title>" << _statusCode << " " << reasonPhrase << "</title>\n"
		 << "  </head>\n"
		 << "  <body>\n"
		 << "    <center><h1>" << _statusCode << " " << reasonPhrase << "</h1></center>\n"
		 << "    <hr><center>webserv/1.0</center>\n"
		 << "  </body>\n"
		 << "</html>";
	return html.str();
}

std::string Response::_buildStatusLine() const
{
	std::stringstream statusLine;

	statusLine << "HTTP/1.1 " << _statusCode << " " << _reasonPhrase << "\r\n";
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
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
		it != _headers.end(); ++it)
		{
			const std::string& key = it->first;
			if (key != "server" && key != "date" && key != "content-type" &&
				key != "content-length" && key != "connection")
					headerStream << key << ": " << it->second << "\r\n";
		}
	return headerStream.str();
}

std::ostream& operator<<(std::ostream& os, const Response& response)
{
	os << "Response:\n";
	os << "- Status: " << response.getStatusCode() << " " << response.getReasonPhrase() << "\n";
	os << "- Headers: " << response.getHeaders().size() << "\n";
	const std::map<std::string, std::string>& headers = response.getHeaders();
	for (std::map<std::string, std::string>::const_iterator it = headers.begin();
		it != headers.end(); ++it)
			os << "  - " << it->first << ": " << it->second << "\n";
	os << "- Body: '" << response.getBody() << "'\n";
	os << "- Body Length: " << response.getBody().length() << "\n";
	os << "- Raw HTTP Response Preview:\n";
	std::string raw = response.stringify();
	os << "  " << raw << std::endl ;
	return os;
}
