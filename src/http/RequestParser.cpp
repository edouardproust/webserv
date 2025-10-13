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

void	RequestParser::parseRequest(Request& request, const std::string& rawRequest)
{
	if (rawRequest.empty()) {
		request.setStatus(PARSE_ERR_BAD_REQUEST);
		return;
	}
	size_t requestStart;
	if (!isValidStart(rawRequest, requestStart)) {
		request.setStatus(PARSE_ERR_BAD_REQUEST);
		return;
	}
	size_t	headersEnd = rawRequest.find("\r\n\r\n", requestStart);
	if (headersEnd == std::string::npos) {
		request.setStatus(PARSE_ERR_BAD_REQUEST);
		return;
	}
	std::string partBeforeBody = rawRequest.substr(requestStart, headersEnd - requestStart);
	size_t requestLineEnd = partBeforeBody.find("\r\n");
	if (requestLineEnd == std::string::npos) {
		request.setStatus(PARSE_ERR_BAD_REQUEST);
		return;
	}
	std::string	requestLine = partBeforeBody.substr(0, requestLineEnd);
	std::string	headersPart = partBeforeBody.substr(requestLineEnd + 2);
	ParseStatus	result = parseRequestLine(request, requestLine);
	if (result != PARSE_SUCCESS) {
		request.setStatus(result);
		return;
	}
	result = parseHeaders(request, headersPart);
	request.setStatus(result);
}

ParseStatus	RequestParser::parseRequestLine(Request& request, const std::string& line)
{

	for (size_t i = 0; i < line.length(); i++)
	{
		if (line[i] != ' ' && std::isspace(line[i]))
			return (PARSE_ERR_BAD_REQUEST);
	}
	std::istringstream	requestLineStream(line);
	std::string	methodStr, path, version;
	if (!(requestLineStream >> methodStr >> path >> version))
		return (PARSE_ERR_BAD_REQUEST);
	char c;
	while (requestLineStream.get(c))
	{
		if (c != ' ')
			 return (PARSE_ERR_BAD_REQUEST);
	}
	if (!isValidMethod(methodStr))
		return (PARSE_ERR_BAD_REQUEST);
	if (!isValidPath(path))
		return (PARSE_ERR_BAD_REQUEST);
	if (!isValidVersion(version))
		return (PARSE_ERR_HTTP_VERSION_NOT_SUPPORTED);
	request.setMethod(methodStr);
	request.setPath(path);
	request.setVersion(version);
	return (PARSE_SUCCESS);
}

ParseStatus RequestParser::parseHeaders(Request& request, const std::string& headersPart)
{
	std::istringstream	headersStream(headersPart);
	std::string	line;

	if (headersPart.empty() && request.getVersion() == "HTTP/1.0")
    	return (PARSE_SUCCESS);
	while (std::getline(headersStream, line))
	{
		if (!line.empty() && line[line.length() - 1] == '\r')
			line.erase(line.length()-1);
		if (line.empty())
			break ;
		if (std::isspace(line[0]))
			return (PARSE_ERR_HEADER_SYNTAX_ERROR);
		ParseStatus result = parseHeaderLine(request, line);
		if (result != PARSE_SUCCESS)
			return (result);
	}
	return (PARSE_SUCCESS);
}

ParseStatus	RequestParser::parseHeaderLine(Request& request, const std::string& line)
{
	size_t	colonPos = line.find(":");
	if (colonPos == std::string::npos)
		return (PARSE_ERR_HEADER_SYNTAX_ERROR);
	std::string	name = line.substr(0, colonPos);
	std::string	value = line.substr(colonPos + 1);
	if (!name.empty() && std::isspace(name[name.length() - 1]))
		return (PARSE_ERR_HEADER_SYNTAX_ERROR);
	if (name.empty())
		return (PARSE_ERR_HEADER_NAME_EMPTY);
	if (!isValidHeaderName(name))
		return (PARSE_ERR_HEADER_SYNTAX_ERROR);
	std::string normalizedName = normalizeHeaderName(name);
	request.addHeader(normalizedName, value);
	return (PARSE_SUCCESS);
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
