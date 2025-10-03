#include "http/RequestParser.hpp"
#include <sstream>
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
	size_t	headersEnd = rawRequest.find("\r\n\r\n");
	if (headersEnd == std::string::npos)
		return (BAD_REQUEST);
	std::string partBeforeBody = rawRequest.substr(0, headersEnd);
	size_t requestLineEnd = partBeforeBody.find("\r\n");
	if (requestLineEnd == std::string::npos)
		return (BAD_REQUEST);
	std::string	requestLine = partBeforeBody.substr(0, requestLineEnd);
	std::string	headersPart = partBeforeBody.substr(requestLineEnd + 2);
	Status	result = parseRequestLine(request, requestLine);
	if (result != SUCCESS)
		return (result);
	if (hasBody(rawRequest) && request.getMethod() == GET)
		return (LENGTH_REQUIRED);
	result = parseHeaders(request, headersPart);
	if (result != SUCCESS)
		return (result);
	return (SUCCESS);
}

Status	RequestParser::parseRequestLine(Request& request, const std::string& line)
{
	std::istringstream	requestLineStream(line);
	std::string	methodStr, path, version;

	if (!(requestLineStream >> methodStr >> path >> version))
		return (BAD_REQUEST);
	std::string extraContent;
	if (requestLineStream >> methodStr >> path >> version >> extraContent)
		return (BAD_REQUEST);
	if (!isValidMethod(methodStr))
		return (METHOD_NOT_ALLOWED);
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
    (void)request; 
    (void)headersPart;
    
    // TODO: Implement header parsing later
    return (SUCCESS);
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

bool	RequestParser::hasBody(const std::string& rawRequest) const
{
	size_t	headersEnd = rawRequest.find("\r\n\r\n");
	if (rawRequest.length() > headersEnd + 4)
		return (true);
	return (false);
}
