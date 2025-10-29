#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include "HttpStatus.hpp"
# include "RequestParser.hpp"
# include <map>

class Response
{
	private:

	HttpStatus	_status;
	std::map<std::string, std::string> _headers;
	std::string _body;

	std::string	_buildStatusLine() const;
	std::string	_buildHeaders() const;
	std::string	_getCurrentDate() const;

	public:

	Response();
	Response(const Response& other);
	Response& operator=(const Response& other);
	~Response();

	const HttpStatus& getStatus() const;
	const std::map<std::string, std::string>& getHeaders() const;
	const std::string& getBody() const;

	void	setStatus(const HttpStatus& statusCode);
	void	setHeader(const std::string& name, const std::string& value);
	void	setBody(const std::string& body);

	std::string	stringify() const;
};

std::ostream& operator<<(std::ostream& os, const Response& response);

#endif
