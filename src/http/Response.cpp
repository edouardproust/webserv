#include "http/Response.hpp"
#include "utils/Utils.hpp"
#include <ctime>

Response::Response() {}

Response::Response(const Response& other)
{
	(void)other;
}

Response& Response::operator=(const Response& other)
{
	(void)other;
	return (*this);
}

Response::~Response() {}

std::string	Response::buildResponse(int statusCode, const std::map<std::string, std::string>& headers,
					const std::string& body)
{
	std::stringstream response;
	std::string reasonPhrase = _getReasonPhrase(statusCode);
	response << _buildStatusLine(statusCode, reasonPhrase);
	std::map<std::string, std::string> allHeaders = headers;
	allHeaders["server"] = "webserv/1.0";
	allHeaders["date"] = _getCurrentDate();
	if (allHeaders.find("content-type") == allHeaders.end() && !body.empty())
		allHeaders["content-type"] = "text/html"; //set text/html as default only if it's not provided in the request
	std::stringstream lengthStream;
	lengthStream << body.length();
	allHeaders["content-length"] = lengthStream.str();
	if (allHeaders.find("connection") == allHeaders.end())
		allHeaders["connection"] = "keep-alive";
	else
	{
		std::string normalizedConn = utils::toLowerCase(allHeaders["connection"]);
		if (normalizedConn != "keep-alive" && normalizedConn != "close")
			allHeaders["connection"] = "keep-alive";
	}
	response << _buildHeaders(allHeaders);
	response << "\r\n";
	if (!body.empty())
		response << body;
	return response.str();
}

std::string Response::buildErrorResponse(int statusCode)
{
	std::map<std::string, std::string> headers;
	headers["Content-Type"] = "text/html";
	std::string body = _generateErrorPage(statusCode);
	return buildResponse(statusCode, headers, body);
}

std::string Response::_buildStatusLine(int statusCode, const std::string& reasonPhrase) const
{
	std::stringstream statusLine;

	statusLine << "HTTP/1.1 " << statusCode << " " << reasonPhrase << "\r\n";
	return statusLine.str();
}

std::string Response::_buildHeaders(const std::map<std::string, std::string>& headers) const
{
	std::stringstream headerStream;

	headerStream << "Server: " << headers.find("server")->second << "\r\n";
	headerStream << "Date: " << headers.find("date")->second << "\r\n";
	if (headers.find("content-type") != headers.end())
		headerStream << "Content-Type: " << headers.find("content-type")->second << "\r\n";
	headerStream << "Content-Length: " << headers.find("content-length")->second << "\r\n";
	headerStream << "Connection: " << headers.find("connection")->second << "\r\n";
	return headerStream.str();
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

std::string Response::_generateErrorPage(int statusCode) const
{
	std::stringstream html;
	std::string reasonPhrase = _getReasonPhrase(statusCode);

	html << "<html>\n"
		 << "  <head>\n"
		 << "    <title>" << statusCode << " " << reasonPhrase << "</title>\n"
		 << "  </head>\n"
		 << "  <body>\n"
		 << "    <center><h1>" << statusCode << " " << reasonPhrase << "</h1></center>\n"
		 << "    <hr><center>webserv/1.0</center>\n"
		 << "  </body>\n"
		 << "</html>";
	return html.str();
}
