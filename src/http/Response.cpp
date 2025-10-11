#include "http/Response.hpp"
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

std::string	Response::buildResponse(int statusCode, const std::map<std::string, std::string>& headers)
{
	std::stringstream response;
	std::string reasonPhrase = _getReasonPhrase(statusCode);
	response << _buildStatusLine(statusCode, reasonPhrase);
	std::map<std::string, std::string> allHeaders = headers;
	if (allHeaders.find("Server") == allHeaders.end())
		allHeaders["Server"] = "webserv";
	if (allHeaders.find("Date") == allHeaders.end())
		allHeaders["Date"] = _getCurrentDate();
	response << _buildHeaders(allHeaders);
	response << "\r\n";
	//body implementation here
	return response.str();
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
	if (headers.find("Server") != headers.end())
		headerStream << "Server: " << headers.find("Server")->second << "\r\n";
	if (headers.find("Date") != headers.end())
		headerStream << "Date: " << headers.find("Date")->second << "\r\n";
	if (headers.find("Content-Type") != headers.end())
		headerStream << "Content-Type: " << headers.find("Content-Type")->second << "\r\n";
	else
		headerStream << "Content-Type: text/html\r\n";
	if (headers.find("Content-Length") != headers.end())
	 	 headerStream << "Content-Length: " << headers.find("Content-Length")->second << "\r\n";
	else
		headerStream << "Content-Length: 0\r\n"; //for now it will always be this until body is implemented
	if (headers.find("Connection") != headers.end())
		headerStream << "Connection: " << headers.find("Connection")->second << "\r\n";
	else
		headerStream << "Connection: close\r\n"; //for now it will always be close for HTTP 1.0
	return headerStream.str();
}

std::string Response::_getReasonPhrase(int statusCode) const
{
	switch(statusCode)
	{
		case 200:
			return "OK";
		case 400:
			return "Bad Request";
		case 404:
			return "Not Found";
		case 405:
			return "Method Not Allowed";
		case 411:
			return "Length Required"; //For missing body-mandatory in HTPP 1.1 for POST
		case 413:
			return "Content Too Large"; //For bigger body than MAX_CLIENT_BODY_SIZE
		case 500:
			return "Internal Server Error";
		case 505:
			return "HTTP Version Not Supported";
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
