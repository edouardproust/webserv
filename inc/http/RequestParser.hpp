#ifndef REQUESTPARSER_HPP
# define REQUESTPARSER_HPP

# include "http/Request.hpp"
# include <string>
# include <sstream>

enum ParseStatus
{
	PARSE_SUCCESS = 200,
	PARSE_ERR_BAD_REQUEST = 400,
	PARSE_ERR_HTTP_VERSION_NOT_SUPPORTED = 505,
	PARSE_ERR_LENGTH_REQUIRED = 411, // eg POST request missing Content-Length
	PARSE_ERR_BODY_TOO_LARGE = 413 // Body exceeds config's max size limit
};

class RequestParser
{
	private :

	ParseStatus	_parseRequestLine(Request& request, const std::string& line);
	ParseStatus	_parseHeaders(Request& request, const std::string& headersPart, bool hasBody);
	ParseStatus	_parseHeaderLine(Request& request, const std::string& line);
	ParseStatus	_validateBody(const Request& request);

	bool	_isValidStart(const std::string& rawRequest, size_t& requestStart) const;
	bool	_isValidMethod(const std::string& _method) const;
	bool	_isValidPath(const std::string& _path) const;
	bool	_isValidVersion(const std::string& _version) const;
	bool	_isValidHeaderName(const std::string& name) const;
	bool	_hasBody(const std::string& rawRequest, size_t headersEnd) const;
	std::string	_normalizeHeaderName(const std::string& name) const;

	public :

	RequestParser();
	RequestParser(const RequestParser& other);
	RequestParser& operator=(const RequestParser& other);
	~RequestParser();

	ParseStatus	parseRequest(Request& request, const std::string& rawRequest);
};

#endif
