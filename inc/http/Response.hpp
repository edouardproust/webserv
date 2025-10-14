#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include "RequestParser.hpp"
# include <map>

class Response
{
	private:

	std::string	_buildStatusLine(int statusCode, const std::string& reasonPhrase) const;
	std::string	_buildHeaders(const std::map<std::string, std::string>& headers) const;
	std::string	_getReasonPhrase(int statusCode) const;
	std::string	_getCurrentDate() const;

	public:

	Response();
	Response(const Response& other);
	Response& operator=(const Response& other);
	~Response();

	std::string	buildResponse(int statusCode, const std::map<std::string, std::string>& headers, const std::string& body);
	std::string	buildErrorResponse(int statusCode); //for later

};

#endif
