#ifndef REQUESTPARSER_HPP
# define REQUESTPARSER_HPP

# include "Request.hpp"
# include <string>
# include <sstream>

enum Status
{
	SUCCESS = 200,
	BAD_REQUEST = 400, // Wrong request line formst
	METHOD_NOT_ALLOWED = 405, // Methods we don't use but normally exist
	HTTP_VERSION_NOT_SUPPORTED = 505, // Not HTTP/1.0 (or HTPP/1.1 for later)
	LENGTH_REQUIRED = 411, // For empty body eg in POST
	HEADER_SYNTAX_ERROR = 400, // Wrong header line
	HEADER_NAME_EMPTY = 400, // Empty header name
	MISSING_HOST_HEADER = 400 //To implement for HTTP 1.1
};

class RequestParser
{
	private :

	Status	parseRequestLine(Request& request, const std::string& line);
	Status	parseHeaders(Request& request, const std::string& headersPart);
	Status	parseHeaderLine(Request& request, const std::string& line);

	bool	isValidStart(const std::string& rawRequest, size_t& requestStart) const;
	Method	methodFromString(const std::string& methodStr);
	bool	isValidMethod(const std::string& method) const;
	bool	isValidPath(const std::string& path) const;
	bool	isValidVersion(const std::string& version) const;
	//bool	hasBody(const std::string& rawRequest) const; -> wrong, should be handled in Response
	bool	hasExtraContent(std::istringstream& line);
	bool	isValidHeaderValue(const std::string& value) const;

	public :

	RequestParser();
	RequestParser(const RequestParser& other);
	RequestParser& operator=(const RequestParser& other);
	~RequestParser();

	Status	parse_request(Request& request, const std::string& rawRequest);
};

#endif
