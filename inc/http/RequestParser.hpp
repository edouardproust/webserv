#ifndef REQUESTPARSER_HPP
# define REQUESTPARSER_HPP

# include "Request.hpp"
# include "HttpStatus.hpp"
# include "constants.hpp"

class RequestParser
{
	private :

	HttpStatus	_parseRequestLine(Request& request, const std::string& line);
	HttpStatus	_parseUri(Request& request, const std::string& uri);
	HttpStatus	_parseUrl(std::string& result, const std::string& encoded);
	HttpStatus	_parseHeaders(Request& request, const std::string& headersPart, bool hasBody);
	HttpStatus	_parseHeaderLine(Request& request, const std::string& line);
	HttpStatus	_parseChunkedBody(Request& request);
	HttpStatus	_validateBody(Request& request);

	bool				_isValidStart(const std::string& rawRequest, size_t& requestStart) const;
	bool				_isValidMethod(const std::string& _method) const;
	bool				_isValidPath(const std::string& _path) const;
	bool				_isValidVersion(const std::string& _version) const;
	bool				_isValidVersionNumber(const std::string& numStr) const;
	bool				_isValidHeaderName(const std::string& name) const;
	bool				_isValidHeaderValue(const std::string& value) const;
	bool				_isValidContentType(const std::string& contentType) const;
	bool				_isValidContentLength(const std::string& contentLength) const;
	bool				_hasBody(const std::string& rawRequest, size_t headersEnd) const;
	std::string			_normalizeHeaderName(const std::string& name) const;

	public :

	RequestParser();
	RequestParser(const RequestParser& other);
	RequestParser& operator=(const RequestParser& other);
	~RequestParser();

	void	parseRequest(Request& request, const std::string& rawRequest);
};

#endif
