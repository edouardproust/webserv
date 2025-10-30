#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include "http/RequestParser.hpp"
# include "http/HttpStatus.hpp"
#include "utils/utils.hpp"
#include <ctime>

class Response
{
	private:

	HttpStatus _status;
	std::map<std::string, std::string> _headers;
	std::string _body;

	std::string _generateErrorPage() const;
	std::string _buildStatusLine() const;
	std::string _buildHeaders() const;

	void _initDefaultHeaders();
	void _parseRawResponse(const std::string& rawResponse);
	int	 _setHeaders(const std::string& headersPart);

	public:

	Response();
	Response(std::string const& rawResponse);
	Response(const Response& other);
	Response& operator=(const Response& other);
	~Response();

	std::string	stringify() const;
	const HttpStatus& getStatus() const;
	const std::map<std::string, std::string>& getHeaders() const;
	const std::string& getBody() const;
	std::string	getCurrentDate() const;
	bool hasHeader(const std::string& keyLowcase) const;


	void setStatus(const HttpStatus& status);
	void setHeader(const std::string& name, const std::string& value);
	void setBody(const std::string& body);
	void setError(const HttpStatus& status);

	class RawException: public std::runtime_error {
		public:
			explicit RawException(const std::string& msg);
	};

};

std::ostream& operator<<(std::ostream& os, const Response& response);

#endif
