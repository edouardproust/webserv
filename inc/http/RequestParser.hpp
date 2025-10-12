#ifndef REQUESTPARSER_HPP
# define REQUESTPARSER_HPP

# include "http/Request.hpp"
# include <string>
# include <sstream>

class RequestParser
{
	private :

	ParseStatus	parseRequestLine(Request& request, const std::string& line);
	ParseStatus	parseHeaders(Request& request, const std::string& headersPart);
	ParseStatus	parseHeaderLine(Request& request, const std::string& line);

	bool		isValidStart(const std::string& rawRequest, size_t& requestStart) const;
	bool		isValidMethod(const std::string& method) const;
	bool		isValidPath(const std::string& path) const;
	bool		isValidVersion(const std::string& version) const;
	bool		isValidHeaderName(const std::string& name) const;
	std::string	normalizeHeaderName(const std::string& name) const;

	public :

	RequestParser();
	RequestParser(const RequestParser& other);
	RequestParser& operator=(const RequestParser& other);
	~RequestParser();

	ParseStatus	parseRequest(Request& request, const std::string& rawRequest);
};

#endif
