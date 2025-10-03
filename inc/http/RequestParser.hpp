#ifndef REQUESTPARSER_HPP
# define REQUESTPARSER_HPP

# include "Request.hpp"
# include <string>

class RequestParser
{
	private :

	bool	parseRequestLine(Request& request, const std::string& line);
	bool	parseHeaders(Request& request, const std::string& headersSection);
	bool	parseHeaderLine(Request& request, const std::string& line);

	bool	isValidMethod(const std::string& method) const;
	bool	isValidVersion(const std::string& version) const;
	bool	isValidPath(const std::string& path) const;

	public :

	RequestParser();
	RequestParser(const RequestParser& other);
	RequestParser& operator=(const RequestParser& other);
	~RequestParser();

	bool	parse_request(Request& request, const std::string& rawRequest);
};

#endif
