#ifndef REQUESTPARSER_HPP
# define REQUESTPARSER_HPP

# include "Request.hpp"
# include <string>

enum Status
{
	SUCCESS = 200,
	BAD_REQUEST = 400, // Wrong request line formst
	METHOD_NOT_ALLOWED = 405, // Methods we won't use or wrong methods
	HTTP_VERSION_NOT_SUPPORTED = 505, // Not HTTP/1.0 (or HTPP/1.1 for later)
	LENGTH_REQUIRED = 411 // GET with body (shouldn't have)
};

class RequestParser
{
	private :

	Status	parseRequestLine(Request& request, const std::string& line);
	Status	parseHeaderLine(Request& request, const std::string& line);
	Status	parseHeaders(Request& request, const std::string& headersSection);

	Method	methodFromString(const std::string& methodStr);
	bool	isValidMethod(const std::string& method) const;
	bool	isValidPath(const std::string& path) const;
	bool	isValidVersion(const std::string& version) const;
	bool	hasBody(const std::string& rawRequest) const;

	public :

	RequestParser();
	RequestParser(const RequestParser& other);
	RequestParser& operator=(const RequestParser& other);
	~RequestParser();

	Status	parse_request(Request& request, const std::string& rawRequest);
};

#endif
