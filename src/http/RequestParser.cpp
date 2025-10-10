#include "http/RequestParser.hpp"
#include "http/HttpException.hpp"
#include <iostream>

RequestParser::RequestParser() {}

RequestParser::RequestParser(const RequestParser& other)
{
	(void)other;
}

RequestParser& RequestParser::operator=(const RequestParser& other)
{
	(void)other;
	return (*this);
}

RequestParser::~RequestParser() {}

void	RequestParser::parse(Request& request, const std::string& rawRequest)
{
	if (rawRequest.empty())
		throw HttpException(400, "Empty request");
	size_t requestStart;
	if (!isValidStart(rawRequest, requestStart))
		throw HttpException(400, "Invalid start of request");
	size_t	headersEnd = rawRequest.find("\r\n\r\n", requestStart);
	if (headersEnd == std::string::npos)
		throw HttpException(400, "Headers not properly terminated");
	std::string partBeforeBody = rawRequest.substr(requestStart, headersEnd - requestStart);
	size_t requestLineEnd = partBeforeBody.find("\r\n");
	if (requestLineEnd == std::string::npos)
		throw HttpException(400, "Request line not properly terminated");
	std::string	requestLine = partBeforeBody.substr(0, requestLineEnd);
	std::string	headersPart = partBeforeBody.substr(requestLineEnd + 2);
	parseRequestLine(request, requestLine);
	parseHeaders(request, headersPart);
}

void	RequestParser::parseRequestLine(Request& request, const std::string& line)
{

	for (size_t i = 0; i < line.length(); i++)
	{
		if (line[i] != ' ' && std::isspace(line[i]))
			throw HttpException(400, "Invalid whitespace in request line");
	}
	std::istringstream	requestLineStream(line);
	std::string	methodStr, path, version;
	if (!(requestLineStream >> methodStr >> path >> version))
		throw HttpException(400, "Malformed request line");
	char c;
	while (requestLineStream.get(c))
	{
		if (c != ' ')
			throw HttpException(400, "Extra characters after HTTP version in request line");
	}
	if (!isValidMethod(methodStr))
		throw HttpException(400, "Invalid HTTP method in request line");
	if (!isValidPath(path))
		throw HttpException(400, "Invalid path in request line");
	if (!isValidVersion(version))
		throw HttpException(505, "Unsupported HTTP version in request line");
	request.setMethod(methodStr);
	request.setPath(path);
	request.setVersion(version);
}

void	RequestParser::parseHeaders(Request& request, const std::string& headersPart)
{
	std::istringstream	headersStream(headersPart);
	std::string	line;

	if (headersPart.empty() && request.getVersion() == "HTTP/1.0")
    	return; // HTTP/1.0 doesn't require Host header
	while (std::getline(headersStream, line))
	{
		if (!line.empty() && line[line.length() - 1] == '\r')
			line.erase(line.length()-1);
		if (line.empty())
			break ;
		if (std::isspace(line[0]))
			throw HttpException(400, "Header line starts with whitespace");
		parseHeaderLine(request, line);
	}
}

void	RequestParser::parseHeaderLine(Request& request, const std::string& line)
{
	size_t	colonPos = line.find(":");
	if (colonPos == std::string::npos)
		throw HttpException(400, "Header line missing ':' separator");
	std::string	name = line.substr(0, colonPos);
	std::string	value = line.substr(colonPos + 1);
	if (!name.empty() && std::isspace(name[name.length() - 1]))
		throw HttpException(400, "Whitespace at end of header name");
	if (name.empty())
		throw HttpException(400, "Empty header name");
	if (!isValidHeaderName(name))
		throw HttpException(400, "Invalid characters in header name");
	if (name == "host" && value.empty())
		throw HttpException(400, "Empty Host header value");
	std::string normalizedName = normalizeHeaderName(name);
	request.addHeader(normalizedName, value);
}

bool	RequestParser::isValidStart(const std::string& rawRequest, size_t& requestStart) const
{
	for (size_t i = 0; i < rawRequest.length(); i++)
	{
		if (rawRequest[i] == '\r' || rawRequest[i] == '\n')
			continue ;
		if (std::isspace(rawRequest[i]))
			return (false);
		requestStart = i;
		return (true);
	}
	return (false);
}

bool	RequestParser::isValidMethod(const std::string& methodStr) const
{
	if (methodStr.empty())
		return (false);
	for (size_t i = 0; i < methodStr.length(); i++)
	{
		if (!std::isalpha(methodStr[i]) || !std::isupper(methodStr[i]))
			return (false);
    }
	return (Request::isSupportedMethod(methodStr));
}

bool	RequestParser::isValidPath(const std::string& path) const
{
	if (path.empty())
		return (false);
	if (path[0] != '/' || path.find(' ') != std::string::npos)
		return (false);
	return (true);
}

bool	RequestParser::isValidVersion(const std::string& version) const
{
	if (version.empty())
		return (false);
	return (version == "HTTP/1.0"); // version == "HTTP/1.1" for later
}

bool	RequestParser::isValidHeaderName(const std::string& name) const
{
	const std::string validChars = "!#$%&'*+-.^_`|~";

	for (size_t i = 0; i < name.length(); i++)
	{
		if (!std::isalnum(name[i]) && validChars.find(name[i]) == std::string::npos)
			return (false);
	}
	return (true);
}

std::string	RequestParser::normalizeHeaderName(const std::string& name) const
{
	std::string normalized = name;
	for (size_t i = 0; i < normalized.length(); ++i)
		normalized[i] = std::tolower(normalized[i]);
	return (normalized);
}
