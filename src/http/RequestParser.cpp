#include "http/RequestParser.hpp"
#include "Constants.hpp"
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

ParseStatus	RequestParser::parseRequest(Request& request, const std::string& rawRequest)
{
	if (rawRequest.empty())
		return PARSE_ERR_BAD_REQUEST;
	size_t requestStart;
	if (!_isValidStart(rawRequest, requestStart))
		return PARSE_ERR_BAD_REQUEST;
	size_t	headersEnd = rawRequest.find("\r\n\r\n", requestStart);
	if (headersEnd == std::string::npos)
		return PARSE_ERR_BAD_REQUEST;
	std::string partBeforeBody = rawRequest.substr(requestStart, headersEnd - requestStart);
	size_t requestLineEnd = partBeforeBody.find("\r\n");
	if (requestLineEnd == std::string::npos)
		return PARSE_ERR_BAD_REQUEST;
	std::string	requestLine = partBeforeBody.substr(0, requestLineEnd);
	std::string	headersPart = partBeforeBody.substr(requestLineEnd + 2);
	ParseStatus	result = _parseRequestLine(request, requestLine);
	if (result != PARSE_SUCCESS)
		return result;
	bool hasBody = _hasBody(rawRequest, headersEnd);
	result = _parseHeaders(request, headersPart, hasBody);
	if (result != PARSE_SUCCESS)
		return result;
	size_t bodyStart = headersEnd + 4;
	if (hasBody)
	{
		std::string body = rawRequest.substr(bodyStart);
		request.setBody(body);
		ParseStatus bodyResult = _validateBody(request);
		if (bodyResult != PARSE_SUCCESS)
			return bodyResult;
	}
	return PARSE_SUCCESS;
}

ParseStatus	RequestParser::_parseRequestLine(Request& request, const std::string& line)
{

	for (size_t i = 0; i < line.length(); i++)
	{
		if (line[i] != ' ' && std::isspace(line[i]))
			return PARSE_ERR_BAD_REQUEST;
	}
	std::istringstream	requestLineStream(line);
	std::string	methodStr, _path, _version;
	if (!(requestLineStream >> methodStr >> _path >> _version))
		return PARSE_ERR_BAD_REQUEST;
	char c;
	while (requestLineStream.get(c))
	{
		if (c != ' ')
			 return PARSE_ERR_BAD_REQUEST;
	}
	if (!_isValidMethod(methodStr))
		return PARSE_ERR_BAD_REQUEST;
	if (!_isValidPath(_path))
		return PARSE_ERR_BAD_REQUEST;
	if (!_isValidVersion(_version))
		return PARSE_ERR_HTTP_VERSION_NOT_SUPPORTED;
	request.setMethod(methodStr);
	request.setPath(_path);
	request.setVersion(_version);
	return PARSE_SUCCESS;
}

ParseStatus RequestParser::_parseHeaders(Request& request, const std::string& headersPart, bool hasBody)
{
	std::istringstream	headersStream(headersPart);
	std::string	line;
	while (std::getline(headersStream, line))
	{
		if (!line.empty() && line[line.length() - 1] == '\r')
			line.erase(line.length()-1);
		if (line.empty())
			break ;
		if (std::isspace(line[0]))
			return PARSE_ERR_BAD_REQUEST;
		ParseStatus result = _parseHeaderLine(request, line);
		if (result != PARSE_SUCCESS)
			return result;
	}	
	std::map<std::string, std::string> headers = request.getHeaders();
	if (headers.find("host") == headers.end())
		return PARSE_ERR_BAD_REQUEST;
	if (hasBody)
	{
		if (headers.find("content-length") == headers.end())
			return PARSE_ERR_LENGTH_REQUIRED;
	}
	return PARSE_SUCCESS;
}

ParseStatus	RequestParser::_parseHeaderLine(Request& request, const std::string& line)
{
	size_t	colonPos = line.find(":");
	if (colonPos == std::string::npos)
		return PARSE_ERR_BAD_REQUEST;
	std::string	name = line.substr(0, colonPos);
	std::string	value = line.substr(colonPos + 1);
	if (!name.empty() && std::isspace(name[name.length() - 1]))
		return PARSE_ERR_BAD_REQUEST;
	if (name.empty())
		return PARSE_ERR_BAD_REQUEST;
	if (!_isValidHeaderName(name))
		return PARSE_ERR_BAD_REQUEST;
	value = _trimOWS(value);
	std::string normalizedName = _normalizeHeaderName(name);
	request.addHeader(normalizedName, value);
	return PARSE_SUCCESS;
}

ParseStatus	RequestParser::_validateBody(const Request& request)
{
	std::map<std::string, std::string> headers = request.getHeaders();

	if (headers.find("content-length") != headers.end())
	{
		std::string contentLengthStr = headers["content-length"];
		unsigned long contentLength;
		std::istringstream contentLengthStream(contentLengthStr);
		if (!(contentLengthStream >> contentLength))
			return PARSE_ERR_BAD_REQUEST;
		if (request.getBody().length() != contentLength)
			return PARSE_ERR_BAD_REQUEST;
	}
	return PARSE_SUCCESS;
}

bool	RequestParser::_isValidStart(const std::string& rawRequest, size_t& requestStart) const
{
	for (size_t i = 0; i < rawRequest.length(); i++)
	{
		if (rawRequest[i] == '\r' || rawRequest[i] == '\n')
			continue ;
		if (std::isspace(rawRequest[i]))
			return false;
		requestStart = i;
		return true;
	}
	return false;
}

bool	RequestParser::_isValidMethod(const std::string& methodStr) const
{
	if (methodStr.empty())
		return false;
	for (size_t i = 0; i < methodStr.length(); i++)
	{
		if (!std::isalpha(methodStr[i]) || !std::isupper(methodStr[i]))
			return false;
    }
	return (Request::isSupportedMethod(methodStr));
}

bool	RequestParser::_isValidPath(const std::string& _path) const
{
	if (_path.empty())
		return false;
	if (_path[0] != '/' || _path.find(' ') != std::string::npos)
		return false;
	return true;
}

bool	RequestParser::_isValidVersion(const std::string& _version) const
{
	if (_version.empty())
		return false;
	 return _version == "HTTP/1.1";
}

bool	RequestParser::_isValidHeaderName(const std::string& name) const
{
	const std::string validChars = "!#$%&'*+-.^_`|~";

	for (size_t i = 0; i < name.length(); i++)
	{
		if (!std::isalnum(name[i]) && validChars.find(name[i]) == std::string::npos)
			return false;
	}
	return true;
}

std::string	 RequestParser::_trimOWS(const std::string& str)
{
	size_t start = 0;
	while (start < str.size() && (str[start] == ' ' || str[start] == '\t'))
		start++;

	size_t end = str.size();
	while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t'))
		end--;
	return str.substr(start, end - start);
}

bool RequestParser::_hasBody(const std::string& rawRequest, size_t headersEnd) const
{
	return headersEnd + 4 < rawRequest.length();
}

std::string	RequestParser::_normalizeHeaderName(const std::string& name) const
{
	std::string normalized = name;
	for (size_t i = 0; i < normalized.length(); ++i)
		normalized[i] = std::tolower(normalized[i]);
	return normalized;
}
