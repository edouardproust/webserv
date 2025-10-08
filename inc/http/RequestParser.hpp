#ifndef REQUESTPARSER_HPP
# define REQUESTPARSER_HPP

# include "Request.hpp"
# include <string>
# include <sstream>

enum ParseStatus
{
	PARSE_SUCCESS,  //200, OK
	PARSE_ERR_BAD_REQUEST, // 400 Wrong request line formst
	PARSE_ERR_HTTP_VERSION_NOT_SUPPORTED, // 505 Not HTTP/1.0 (or HTPP/1.1 for later)
	PARSE_ERR_LENGTH_REQUIRED,  //411 For empty body eg in POST will be used later when body is parsed
	PARSE_ERR_HEADER_SYNTAX_ERROR, // 400 Wrong header line
	PARSE_ERR_HEADER_NAME_EMPTY, // 400 Empty header name
	PARSE_ERR_MISSING_HOST_HEADER // 400 To implement for HTTP 1.1
};

class RequestParser
{
	private :

	ParseStatus	parseRequestLine(Request& request, const std::string& line);
	ParseStatus	parseHeaders(Request& request, const std::string& headersPart);
	ParseStatus	parseHeaderLine(Request& request, const std::string& line);

	bool	isValidStart(const std::string& rawRequest, size_t& requestStart) const;
	Method	methodFromString(const std::string& methodStr);
	bool	isValidMethod(const std::string& method) const;
	bool	isValidPath(const std::string& path) const;
	bool	isValidVersion(const std::string& version) const;
	bool	isValidHeaderName(const std::string& name) const;
	bool	isInvalidAfterColon(const std::string& value) const;
	std::string	normalizeHeaderName(const std::string& name) const;

	public :

	RequestParser();
	RequestParser(const RequestParser& other);
	RequestParser& operator=(const RequestParser& other);
	~RequestParser();

	ParseStatus	parse_request(Request& request, const std::string& rawRequest);
};

#endif
