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
	allHeaders["Server"] = "webserv";
	allHeaders["Date"] = _getCurrentDate();
	if (allHeaders.find("Content-Type") == allHeaders.end() && !body.empty())
		allHeaders["Content-Type"] = "text/html"; //set text/html as default only if it's not provided in the request
	std::stringstream lengthStream;
	lengthStream << body.length();
	allHeaders["Content-Length"] = lengthStream.str();
	if (allHeaders.find("Connection") == allHeaders.end())
		allHeaders["Connection"] = "keep-alive";
	else
	{
		std::string normalizedConn = utils::toLowerCase(allHeaders["Connection"]);
		if (normalizedConn != "keep-alive" && normalizedConn != "close")
			allHeaders["Connection"] = "keep-alive";
	}
	response << _buildHeaders(allHeaders);
	response << "\r\n";
	if (!body.empty())
		response << body;
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
	headerStream << "Server: " << headers.find("Server")->second << "\r\n";
	headerStream << "Date: " << headers.find("Date")->second << "\r\n";
	if (headers.find("Content-Type") != headers.end())
		headerStream << "Content-Type: " << headers.find("Content-Type")->second << "\r\n";
	headerStream << "Content-Length: " << headers.find("Content-Length")->second << "\r\n";
	headerStream << "Connection: " << headers.find("Connection")->second << "\r\n";
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
			return "Length Required";
		case 413:
			return "Content Too Large";
		case 500:
			return "Internal Server Error";
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
