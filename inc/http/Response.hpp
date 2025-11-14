#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include "http/RequestParser.hpp"
# include "http/HttpStatus.hpp"
#include "utils/utils.hpp"
#include <ctime>

class Response
{
	private:

	HttpStatus	_status;
	std::map<std::string, std::string> _headers;
	std::string _body;

	void		_updateContentLength();
	void		_manageContentType();
	std::string	_getCurrentDate() const;
	std::string	_buildStatusLine() const;
	std::string	_buildHeaders() const;
	void		_initDefaultHeaders();
	void		_parseRawResponse(const std::string& rawResponse);
	int			_setHeaders(const std::string& headersPart);
	bool		_hasHeader(const std::string& keyLowcase) const;

	public:

	Response();
	Response(std::string const& rawResponse);
	Response(const Response& other);
	Response& operator=(const Response& other);
	~Response();

	std::string stringify() const;

	void	setStatus(const HttpStatus& status);
	void	setContentType(const std::string& value);
	void	setHeader(const std::string& name, const std::string& value);
	void	setBody(const std::string& body);
	void	clearBody();
	void	setConnectionFromRequest(const Request& request);

	const HttpStatus& getStatus() const;
	const std::map<std::string, std::string>& getHeaders() const;
	const std::string& getBody() const;

	class RawException: public std::runtime_error {
		public:
			RawException(const std::string& msg);
	};

};

std::ostream& operator<<(std::ostream& os, const Response& response);

#endif
