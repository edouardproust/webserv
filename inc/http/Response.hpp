#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include "RequestParser.hpp"
# include <map>

class Response
{
	private:

	int	_statusCode;
	std::string	_reasonPhrase;
	std::map<std::string, std::string> _headers;
	std::string _body;

	std::string	_getReasonPhrase(int statusCode) const;
	std::string	_getCurrentDate() const;
	std::string _generateErrorPage() const;
	std::string	_buildStatusLine() const;
	std::string	_buildHeaders() const;

	public:

	Response();
	Response(const Response& other);
	Response& operator=(const Response& other);
	~Response();

	int	getStatusCode() const;
	const std::string& getReasonPhrase() const;
	const std::map<std::string, std::string>& getHeaders() const;
	const std::string& getBody() const;

	void	setStatusCode(int statusCode);
	void	setHeader(const std::string& name, const std::string& value);
	void	setBody(const std::string& body);
	void	setError(int statusCode);

	std::string	stringify() const;
};

std::ostream& operator<<(std::ostream& os, const Response& response);

#endif
