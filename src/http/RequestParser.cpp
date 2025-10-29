#include "http/RequestParser.hpp"
#include "utils/utils.hpp"
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
	if (rawRequest.empty())
		return request.setStatus(HttpStatus(400));
	size_t requestStart;
	if (!_isValidStart(rawRequest, requestStart))
		return request.setStatus(HttpStatus(400));
	size_t	headersEnd = rawRequest.find("\r\n\r\n", requestStart);
	if (headersEnd == std::string::npos)
		return request.setStatus(HttpStatus(400));
	std::string partBeforeBody = rawRequest.substr(requestStart, headersEnd - requestStart);
	size_t requestLineEnd = partBeforeBody.find("\r\n");
	if (requestLineEnd == std::string::npos)
		return request.setStatus(HttpStatus(400));
	std::string	requestLine = partBeforeBody.substr(0, requestLineEnd);
	std::string	headersPart = partBeforeBody.substr(requestLineEnd + 2);
	HttpStatus	result = _parseRequestLine(request, requestLine);
	if (result.getCode() != 200)
		return request.setStatus(result);
	bool hasBody = _hasBody(rawRequest, headersEnd);
	result = _parseHeaders(request, headersPart, hasBody);
	if (result.getCode() != 200)
		return request.setStatus(result);
	size_t bodyStart = headersEnd + 4;
	if (hasBody)
	{
		std::string body = rawRequest.substr(bodyStart);
		request.setBody(body);
		HttpStatus bodyResult = _validateBody(request);
		if (bodyResult.getCode() != 200)
			return request.setStatus(bodyResult);
	}
	request.setStatus(HttpStatus(200));
}

HttpStatus	RequestParser::_parseRequestLine(Request& request, const std::string& line)
{
	for (size_t i = 0; i < line.length(); i++)
	{
		if (line[i] != ' ' && std::isspace(line[i]))
			return HttpStatus(400);
	}
	std::istringstream	requestLineStream(line);
	std::string	methodStr, _requestTarget, versionStr;
	if (!(requestLineStream >> methodStr >> _requestTarget >> versionStr))
		return HttpStatus(400);
	char c;
	while (requestLineStream.get(c))
	{
		if (c != ' ')
			return HttpStatus(400);
	}
	HttpStatus result = _parseRequestTarget(request, _requestTarget);
	if (result.getCode() != 200)
	 	return result;
	if (!_isValidMethod(methodStr))
		return HttpStatus(400);
	if (Request::isExistingMethod(methodStr))
		return HttpStatus(501);
	if (!_isValidVersion(versionStr))
		return HttpStatus(400);;
	if (!Request::isSupportedVersion(versionStr))
		return HttpStatus(505);
	request.setMethod(methodStr);
	request.setVersion(versionStr);
	return HttpStatus(200);
}

HttpStatus	RequestParser::_parseRequestTarget(Request& request, const std::string& _requestTarget)
{
	request.setRequestTarget(_requestTarget);
	size_t queryPos = _requestTarget.find('?');
	if (queryPos != std::string::npos)
	{
		std::string path = _requestTarget.substr(0, queryPos);
		std::string query = _requestTarget.substr(queryPos + 1);
		if (!_isValidPath(path))
			return HttpStatus(400);
		std::string decodedPath, decodedQuery;
		if (_parseUrl(decodedPath, path).getCode() != 200)
			return HttpStatus(400);
		if (_parseUrl(decodedQuery, query).getCode() != 200)
			return HttpStatus(400);
		request.setPath(decodedPath);
		request.setQueryString(decodedQuery);
	}
	else
	{
		if (!_isValidPath(_requestTarget))
			return HttpStatus(400);
		std::string decodedPath;
		if (_parseUrl(decodedPath, _requestTarget).getCode() != 200)
			return HttpStatus(400);
		request.setPath(decodedPath);
		request.setQueryString("");
	}
	return HttpStatus(200);
}

HttpStatus RequestParser::_parseUrl(std::string& result, const std::string& encoded)
{
	result.clear();
	for (size_t i = 0; i < encoded.length(); i++)
	{
		if (encoded[i] == '%' && i + 2 < encoded.length())
		{
			std::string hex = encoded.substr(i + 1, 2);
			if (!std::isxdigit(hex[0]) || !std::isxdigit(hex[1]))
				return HttpStatus(400);
			char decodedChar = utils::hexToChar(hex);
			if (decodedChar == '\0' || decodedChar == '\r' || decodedChar == '\n' ||
				decodedChar == '\t' || decodedChar == '\v' || decodedChar == '\f')
				return HttpStatus(400);
			result += decodedChar;
			i += 2;
		}
		else if (encoded[i] == '+')
			result += ' ';
		else
			result += encoded[i];
	}
	return HttpStatus(200);
}

HttpStatus RequestParser::_parseHeaders(Request& request, const std::string& headersPart, bool hasBody)
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
			return HttpStatus(400);
		HttpStatus result = _parseHeaderLine(request, line);
		if (result.getCode() != 200)
			return result;
	}
	std::map<std::string, std::string> headers = request.getHeaders();
	if (headers.find("content-length") != headers.end() &&
		headers.find("transfer-encoding") != headers.end())
			return HttpStatus(400);
	if (headers.find("host") == headers.end())
		return HttpStatus(400);
	if (hasBody)
	{
		if ((headers.find("content-length") == headers.end()) &&
			headers.find("transfer-encoding") == headers.end())
			return HttpStatus(411);
	}
	return HttpStatus(200);
}

HttpStatus	RequestParser::_parseHeaderLine(Request& request, const std::string& line)
{
	size_t	colonPos = line.find(":");
	if (colonPos == std::string::npos)
		return HttpStatus(400);
	std::string	name = line.substr(0, colonPos);
	std::string	value = line.substr(colonPos + 1);
	if (!name.empty() && std::isspace(name[name.length() - 1]))
		return HttpStatus(400);
	if (name.empty())
		return HttpStatus(400);
	if (!_isValidHeaderName(name))
		return HttpStatus(400);
	value = _trimOWS(value);
	if (!_isValidHeaderValue(value))
		return HttpStatus(400);
	std::string normalizedName = _normalizeHeaderName(name);
	if (normalizedName == "content-type")
	{
		if (!_isValidContentType(value))
			return HttpStatus(400);
		request.setContentType(value);
	}
	if (normalizedName == "transfer-encoding")
	{
		std::string normalizedValue = utils::toLowerCase(value);
		if (normalizedValue == "gzip" || normalizedValue == "deflate" ||
			normalizedValue == "compress" || normalizedValue == "br")
			return HttpStatus(501);
		else if (normalizedValue != "chunked")
			return HttpStatus(400);
	}
	request.addHeader(normalizedName, value);
	return HttpStatus(200);
}

HttpStatus	RequestParser::_parseChunkedBody(Request& request)
{
	std::string chunkedData = request.getBody();
	std::stringstream unchunkedBody;
	size_t pos = 0;
	const size_t dataLength = chunkedData.length();

	while (pos < dataLength)
	{
		size_t lineEnd = chunkedData.find("\r\n", pos);
		if (lineEnd == std::string::npos)
			return HttpStatus(400);
		std::string chunkSizeStr = chunkedData.substr(pos, lineEnd - pos);
		if (chunkSizeStr.empty())
			return HttpStatus(400);
		for (size_t i = 0; i < chunkSizeStr.length(); i++)
		{
			if (!std::isxdigit(chunkSizeStr[i]))
				return HttpStatus(400);
		}
		size_t chunkSize = utils::hexToSizeT(chunkSizeStr);
		if (chunkSize == static_cast<size_t>(-1))
			return HttpStatus(400);
		pos = lineEnd + 2;
		if (chunkSize == 0)
		 	break ;
		if (pos + chunkSize + 2 > dataLength)
			return HttpStatus(400);
		std::string chunkData = chunkedData.substr(pos, chunkSize);
		unchunkedBody << chunkData;
		size_t dataEnd = pos + chunkSize;
		if (chunkedData.substr(dataEnd, 2) != "\r\n")
			return HttpStatus(400);
		pos = dataEnd + 2;
	}
	if (pos + 2 != dataLength || chunkedData.substr(pos, 2) != "\r\n")
		return HttpStatus(400);
	request.setBody(unchunkedBody.str());
	return HttpStatus(200);
}

HttpStatus	RequestParser::_validateBody(Request& request)
{
	std::map<std::string, std::string> headers = request.getHeaders();

	if (headers.find("content-length") != headers.end())
	{
		std::string contentLengthStr = headers["content-length"];
		unsigned long contentLength;
		std::istringstream contentLengthStream(contentLengthStr);
		if (!(contentLengthStream >> contentLength))
			return HttpStatus(400);
		if (request.getBody().length() != contentLength)
			return HttpStatus(400);
	}
	if (headers.find("transfer-encoding") != headers.end())
			return _parseChunkedBody(request);
	return HttpStatus(200);
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
	return (Request::isSupportedMethod(methodStr) || Request::isExistingMethod(methodStr));
}

bool	RequestParser::_isValidPath(const std::string& _path) const
{
	if (_path.empty())
		return false;
	if (_path[0] != '/' || _path.find(' ') != std::string::npos)
		return false;
	return true;
}

bool	RequestParser::_isValidVersion(const std::string& versionStr) const
{
	if (versionStr.empty())
		return false;
	if (versionStr.compare(0, 5, "HTTP/") != 0)
		return false;
	if (versionStr.length() <= 5)
		return false;
	std::string versionNum = versionStr.substr(5);
	for (size_t i = 0; i < versionNum.length(); i++)
	{
		if (!std::isdigit(versionNum[i]) && versionNum[i] != '.')
			return false;
	}
	return true;
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

bool	RequestParser::_isValidHeaderValue(const std::string& value) const
{
	for (size_t i = 0; i < value.length(); i++)
	{
		if (value[i] == '\0')
			return false;
		if (value[i] < 0x20 || value[i] == 0x7F)
			return false;
	}
	return true;
}

bool	RequestParser::_isValidContentType(const std::string& contentType) const
{
	if (contentType.empty())
		return false;
	size_t slashPos = contentType.find('/');
	if (slashPos == std::string::npos || slashPos == 0)
		return false;
	if (contentType.find('\0') != std::string::npos)
		return false;
	return true;
}

bool	RequestParser::_isValidContentLength(const std::string& contentLength) const
{
	if (contentLength.empty())
		return false;
	for (size_t i = 0; i < contentLength.length(); i++)
	{
		if (contentLength[i] < '0' || contentLength[i] > '9')
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

std::string RequestParser::_normalizeHeaderName(const std::string& name) const
{
	return utils::toLowerCase(name);
}
