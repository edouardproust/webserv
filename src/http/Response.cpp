#include "http/Response.hpp"
#include "utils/utils.hpp"
#include <ctime>

Response::Response() : _status(HttpStatus("ok"))
{
	_headers["server"] = SERVER_SOFTWARE;
	_headers["date"] = getCurrentDate();
	_headers["connection"] = "keep-alive";
	_headers["content-length"] = "0";
}

Response::Response(const Response& other)
	: _status(other._status),
	  _reasonPhrase(other._reasonPhrase),
	  _headers(other._headers),
	  _body(other._body) {}

Response& Response::operator=(const Response& other)
{
	if (this != &other)
	{
		_status = other._status;
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

void	Response::setError(const HttpStatus& status)
{
	setStatus(status);
	setHeader("Content-Type", "text/html");
	std::string errorBody = _generateErrorPage();
	setBody(errorBody);
}

std::string Response::getCurrentDate() const
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

	html << "<html>\n"
		 << "  <head>\n"
		 << "    <title>" << _status.toString() << "</title>\n"
		 << "  </head>\n"
		 << "  <body>\n"
		 << "    <center><h1>" << _status.toString() << "</h1></center>\n"
		 << "    <hr><center>" << SERVER_SOFTWARE << "</center>\n"
		 << "  </body>\n"
		 << "</html>";
	return html.str();
}

std::string Response::_buildStatusLine() const
{
	std::stringstream statusLine;

	statusLine << "HTTP/1.1 " << _status.toString() << "\r\n";
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
	os << "- Status: " << response.getStatus().toString() << "\n";
	os << "- Headers: " << response.getHeaders().size() << "\n";
	const std::map<std::string, std::string>& headers = response.getHeaders();
	for (std::map<std::string, std::string>::const_iterator it = headers.begin();
		it != headers.end(); ++it)
			os << "  - " << it->first << ": " << it->second << "\n";
	os << "- Body: '" << response.getBody().empty() << "'\n";
	os << "- Body Length: " << response.getBody().length() << "\n";
	os << "- Raw HTTP Response Preview:\n[" << response.stringify() << "]\n";
	return os;
}
