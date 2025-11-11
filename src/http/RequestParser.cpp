#include "http/RequestParser.hpp"
#include "utils/utils.hpp"
#include "constants.hpp"
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
	std::cout << "RAW REQUEST DATA: '" << rawRequest << "'" << std::endl; // DEBUG
	if (rawRequest.empty())
		return request.setStatus(HttpStatus("bad_request"));
	size_t requestStart;
	if (!_isValidStart(rawRequest, requestStart))
		return request.setStatus(HttpStatus("bad_request"));
	size_t	headersEnd = rawRequest.find("\r\n\r\n", requestStart);
	if (headersEnd == std::string::npos)
		return request.setStatus(HttpStatus("bad_request"));
	std::string partBeforeBody = rawRequest.substr(requestStart, headersEnd + 2 - requestStart);
	size_t requestLineEnd = partBeforeBody.find("\r\n");
	if (requestLineEnd == std::string::npos)
		return request.setStatus(HttpStatus("bad_request"));
	std::string	requestLine = partBeforeBody.substr(0, requestLineEnd);
	std::string	headersPart = partBeforeBody.substr(requestLineEnd + 2);
	HttpStatus	result = _parseRequestLine(request, requestLine);
	if (result.getSlug() != "ok")
		return request.setStatus(result);
	bool hasBody = _hasBody(rawRequest, headersEnd);
	result = _parseHeaders(request, headersPart, hasBody);
	if (result.getSlug() != "ok")
		return request.setStatus(result);
	size_t bodyStart = headersEnd + 4;
	bool hasBodyHeaders = _headersIndicateBody(request);
	if (hasBody || hasBodyHeaders)
	{
		std::string body = rawRequest.substr(bodyStart);
		request.setBody(body);
		HttpStatus bodyResult = _validateBody(request);
		if (bodyResult.getSlug() != "ok")
			return request.setStatus(bodyResult);
	}
	request.setStatus(HttpStatus("ok"));
}

HttpStatus	RequestParser::_parseRequestLine(Request& request, const std::string& line)
{
	for (size_t i = 0; i < line.length(); i++)
	{
		if (line[i] != ' ' && std::isspace(line[i]))
			return HttpStatus("bad_request");
	}
	std::istringstream	requestLineStream(line);
	std::string	methodStr, uri, versionStr;
	if (!(requestLineStream >> methodStr >> uri >> versionStr))
		return HttpStatus("bad_request");
	char c;
	while (requestLineStream.get(c))
	{
		if (c != ' ')
			return HttpStatus("bad_request");
	}
	if (uri.length() > PATH_MAX)
		return HttpStatus("uri_too_long");
	HttpStatus result = _parseUri(request, uri);
	if (result.getSlug() != "ok")
	 	return result;
	if (!_isValidVersion(versionStr))
		return HttpStatus("bad_request");
	if (versionStr != "HTTP/1.1")
		return HttpStatus("version_not_supported");
	if (!_isValidMethod(methodStr))
		return HttpStatus("bad_request");
	if (Request::isExistingMethod(methodStr))
		return HttpStatus("not_implemented");
	request.setMethod(methodStr);
	request.setVersion(versionStr);
	return HttpStatus("ok");
}

HttpStatus	RequestParser::_parseUri(Request& request, const std::string& uri)
{
	request.setUri(uri);
	size_t queryPos = uri.find('?');
	if (queryPos != std::string::npos)
	{
		std::string path = uri.substr(0, queryPos);
		std::string query = uri.substr(queryPos + 1);
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
		if (!_isValidPath(uri))
			return HttpStatus(400);
		std::string decodedPath;
		if (_parseUrl(decodedPath, uri).getCode() != 200)
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
				return HttpStatus("bad_request");
			char decodedChar = utils::hexToChar(hex);
			if (decodedChar == '\0' || decodedChar == '\r' || decodedChar == '\n' ||
				decodedChar == '\t' || decodedChar == '\v' || decodedChar == '\f')
				return HttpStatus("bad_request");
			result += decodedChar;
			i += 2;
		}
		else if (encoded[i] == '+')
			result += ' ';
		else
			result += encoded[i];
	}
	return HttpStatus("ok");
}

HttpStatus RequestParser::_parseHeaders(Request& request, const std::string& headersPart, bool hasBody)
{
	std::istringstream	headersStream(headersPart);
	std::string	line;
	while (std::getline(headersStream, line))
	{
		if (line.length() > HEADER_MAX_SIZE)
			return HttpStatus("request_header_fields_too_large");
		if (!line.empty() && line[line.length() - 1] == '\r')
			line.erase(line.length()-1);
		if (line.empty())
			break ;
		if (std::isspace(line[0]))
			return HttpStatus("bad_request");
		HttpStatus result = _parseHeaderLine(request, line);
		if (result.getSlug() != "ok")
			return result;
	}
	std::map<std::string, std::string> headers = request.getHeaders();
	if (headers.find("content-length") != headers.end() &&
		headers.find("transfer-encoding") != headers.end())
			return HttpStatus("bad_request");
	if (headers.find("host") == headers.end())
		return HttpStatus("bad_request");
	if (hasBody)
	{
		if ((headers.find("content-length") == headers.end()) &&
			headers.find("transfer-encoding") == headers.end())
			return HttpStatus("length_required");
	}
	return HttpStatus("ok");
}

HttpStatus	RequestParser::_parseHeaderLine(Request& request, const std::string& line)
{
	size_t	colonPos = line.find(":");
	if (colonPos == std::string::npos)
		return HttpStatus("bad_request");
	std::string	name = line.substr(0, colonPos);
	std::string	value = line.substr(colonPos + 1);
	if (!name.empty() && std::isspace(name[name.length() - 1]))
		return HttpStatus("bad_request");
	if (name.empty())
		return HttpStatus("bad_request");
	if (!_isValidHeaderName(name))
		return HttpStatus("bad_request");
	value = utils::trim(value);
	if (!_isValidHeaderValue(value))
		return HttpStatus("bad_request");
	std::string normalizedName = _normalizeHeaderName(name);
	if (normalizedName == "content-length")
	{
		if (!_isValidContentLength(value))
			return HttpStatus("bad_request");
	}
	if (normalizedName == "content-type")
	{
		if (!_isValidContentType(value))
			return HttpStatus("bad_request");
		request.setContentType(value);
	}
	if (normalizedName == "transfer-encoding")
	{
		std::string normalizedValue = utils::toLowerCase(value);
		if (normalizedValue == "gzip" || normalizedValue == "deflate" ||
			normalizedValue == "compress" || normalizedValue == "br")
			return HttpStatus("not_implemented");
		else if (normalizedValue != "chunked")
			return HttpStatus("bad_request");
	}
	request.addHeader(normalizedName, value);
	return HttpStatus("ok");
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
			return HttpStatus("bad_request");
		std::string chunkSizeStr = chunkedData.substr(pos, lineEnd - pos);
		if (chunkSizeStr.empty())
			return HttpStatus("bad_request");
		for (size_t i = 0; i < chunkSizeStr.length(); i++)
		{
			if (!std::isxdigit(chunkSizeStr[i]))
				return HttpStatus("bad_request");
		}
		size_t chunkSize = utils::hexToSizeT(chunkSizeStr);
		if (chunkSize == static_cast<size_t>(-1))
			return HttpStatus("bad_request");
		pos = lineEnd + 2;
		if (chunkSize == 0)
		 	break ;
		if (pos + chunkSize + 2 > dataLength)
			return HttpStatus("bad_request");
		std::string chunkData = chunkedData.substr(pos, chunkSize);
		unchunkedBody << chunkData;
		size_t dataEnd = pos + chunkSize;
		if (chunkedData.substr(dataEnd, 2) != "\r\n")
			return HttpStatus("bad_request");
		pos = dataEnd + 2;
	}
	if (pos + 2 != dataLength || chunkedData.substr(pos, 2) != "\r\n")
		return HttpStatus("bad_request");
	request.setBody(unchunkedBody.str());
	return HttpStatus("ok");
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
			return HttpStatus("bad_request");
		if (request.getBody().length() != contentLength)
			return HttpStatus("bad_request");
	}
	if (headers.find("transfer-encoding") != headers.end())
			return _parseChunkedBody(request);
	return HttpStatus("ok");
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
	if (versionNum.empty())
		return false;
	if (versionNum[0] == '.' || versionNum[versionNum.length() - 1] == '.')
		return false;
	size_t dotPos = versionNum.find('.');
	if (dotPos != std::string::npos)
	{
		if (dotPos == 0 || dotPos == versionNum.length() - 1)
			return false;
		std::string majorStr = versionNum.substr(0, dotPos);
		std::string minorStr = versionNum.substr(dotPos + 1);
		if (minorStr.find('.') != std::string::npos)
			return false;
		if (!_isValidVersionNumber(majorStr) || !_isValidVersionNumber(minorStr))
			return false;
	}
	else
	{
		if (!_isValidVersionNumber(versionNum))
			return false;
	}
	return true;
}

bool	RequestParser::_isValidVersionNumber(const std::string& numStr) const
{
	if (numStr.empty())
		return false;
	for (size_t i = 0; i < numStr.length(); i++)
	{
		if (!std::isdigit(numStr[i]))
			return false;
	}
	if (numStr.length() > 1 && numStr[0] == '0')
			return false;
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

bool	RequestParser::_headersIndicateBody(const Request& request) const
{
	const std::map<std::string, std::string>& headers = request.getHeaders();
	return (headers.find("content-length") != headers.end() || 
			headers.find("transfer-encoding") != headers.end());
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

bool RequestParser::_hasBody(const std::string& rawRequest, size_t headersEnd) const
{
	return headersEnd + 4 < rawRequest.length();
}

std::string RequestParser::_normalizeHeaderName(const std::string& name) const
{
	return utils::toLowerCase(name);
}
