#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include "http/RequestParser.hpp"
# include "http/HttpStatus.hpp"
# include <map>

class Response
{
	private:

	HttpStatus	_status;
	std::string	_reasonPhrase;
	std::map<std::string, std::string> _headers;
	std::string _body;

	std::string _generateErrorPage() const;
	std::string	_buildStatusLine() const;
	std::string	_buildHeaders() const;

	public:

	Response();
	Response(const Response& other);
	Response& operator=(const Response& other);
	~Response();

	const HttpStatus& getStatus() const;
	const std::map<std::string, std::string>& getHeaders() const;
	const std::string& getBody() const;
	std::string	getCurrentDate() const;

	void	setStatus(const HttpStatus& status);
	void	setHeader(const std::string& name, const std::string& value);
	void	setBody(const std::string& body);
	void	setError(const HttpStatus& status);

	std::string	stringify() const;
};

std::ostream& operator<<(std::ostream& os, const Response& response);

#endif
