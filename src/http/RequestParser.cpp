#include "http/RequestParser.hpp"
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

Status	RequestParser::parse_request(Request& request, const std::string& rawRequest)
{
	if (rawRequest.empty())
		return (BAD_REQUEST);
	size_t requestStart;
	if (!isValidStart(rawRequest, requestStart))
		return (BAD_REQUEST);
	size_t	headersEnd = rawRequest.find("\r\n\r\n", requestStart);
	if (headersEnd == std::string::npos)
		return (BAD_REQUEST);
	std::string partBeforeBody = rawRequest.substr(requestStart, headersEnd - requestStart);
	size_t requestLineEnd = partBeforeBody.find("\r\n");
	if (requestLineEnd == std::string::npos)
		return (BAD_REQUEST);
	std::string	requestLine = partBeforeBody.substr(0, requestLineEnd);
	std::string	headersPart = partBeforeBody.substr(requestLineEnd + 2);
	Status	result = parseRequestLine(request, requestLine);
	if (result != SUCCESS)
		return (result);
	result = parseHeaders(request, headersPart);
	if (result != SUCCESS)
		return (result);
	return (SUCCESS);
}

Status	RequestParser::parseRequestLine(Request& request, const std::string& line)
{

	for (size_t i = 0; i < line.length(); i++)
	{
		if (line[i] != ' ' && std::isspace(line[i]))
			return (BAD_REQUEST);
	}
	std::istringstream	requestLineStream(line);
	std::string	methodStr, path, version;
	if (!(requestLineStream >> methodStr >> path >> version))
		return (BAD_REQUEST);
	char c;
	while (requestLineStream.get(c))
	{
		if (c != ' ')
			 return (BAD_REQUEST);
	}
	if (!isValidMethod(methodStr))
		return (BAD_REQUEST);
	if (!isValidPath(path))
		return (BAD_REQUEST);
	if (!isValidVersion(version))
		return (HTTP_VERSION_NOT_SUPPORTED);
	request.setMethod(methodFromString(methodStr));
	request.setPath(path);
	request.setVersion(version);
	return (SUCCESS);
}

Status RequestParser::parseHeaders(Request& request, const std::string& headersPart)
{
	std::istringstream	headersStream(headersPart);
	std::string	line;

	if (headersPart.empty() && request.getVersion() == "HTTP/1.0")
    	return (SUCCESS);
	while (std::getline(headersStream, line))
	{
		if (!line.empty() && line[line.length() - 1] == '\r')
			line.erase(line.length()-1);
		if (line.empty())
			break ;
		if (std::isspace(line[0]))
			return (HEADER_SYNTAX_ERROR);
		Status result = parseHeaderLine(request, line);
		if (result != SUCCESS)
			return (result);
	}
	return (SUCCESS);
}

Status	RequestParser::parseHeaderLine(Request& request, const std::string& line)
{
	size_t	colonPos = line.find(":");
	if (colonPos == std::string::npos)
		return (HEADER_SYNTAX_ERROR);
	std::string	name = line.substr(0, colonPos);
	std::string	value = line.substr(colonPos + 1);
	if (name.empty())
		return (HEADER_NAME_EMPTY);
	for (size_t i = 0; i < name.length(); i++)
	{
		if (!std::isalnum(name[i]) && name[i] != '-')
			return (HEADER_SYNTAX_ERROR);
	}
	if (!isValidHeaderValue(value))
		return (HEADER_SYNTAX_ERROR);
	request.addHeader(name, value);
	return (SUCCESS);
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

Method RequestParser::methodFromString(const std::string& methodStr)
{
	if (methodStr == "GET")
		return (GET);
	if (methodStr == "POST")
		return (POST);
	if (methodStr == "DELETE")
		return (DELETE);
	return (UNKNOWN);
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
	return (methodStr == "GET" || methodStr == "POST" || methodStr == "DELETE");
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

bool	RequestParser::isValidHeaderValue(const std::string& value) const
{
	for (size_t i = 0; i < value.length(); i++)
	{
		if (value[i] < 32 && value[i] != '\t')
			return (false);
		if (value[i] == 127)
			return (false);
	}
	return (true);
}
