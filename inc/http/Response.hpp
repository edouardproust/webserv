#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include "http/RequestParser.hpp"
# include "http/HttpStatus.hpp"
#include "utils/utils.hpp"
#include <ctime>

/**
 * Represents an HTTP response with status, headers, and body.
 *
 * Provides utilities to build, modify, stringify, and parse raw HTTP responses.
 * Value-type class following the orthodox canonical form.
 */
class Response
{
	HttpStatus							_status;
	std::map<std::string, std::string>	_headers;
	std::string							_body;

	void		_updateContentLength();
	void		_manageContentType();
	std::string	_buildStatusLine() const;
	std::string	_buildHeaders() const;
	void		_initDefaultHeaders();
	void		_parseRawResponse(std::string const& rawResponse);
	int			_setHeaders(std::string const& headersPart);
	bool		_hasHeader(std::string const& keyLowcase) const;

	public:

 		// Othodox canonical form
		Response();
		Response(std::string const& rawResponse);
		Response(Response const& other);
		Response& operator=(Response const& other);
		~Response();

		std::string stringify() const;

		void	setStatus(HttpStatus const& status);
		void	setContentType(std::string const& value);
		void	setHeader(std::string const& name, std::string const& value);
		void	setBody(std::string const& body);
		void	clearBody();
		void	setConnectionFromRequest(Request const& request);
		bool	isConnectionClose() const;

		HttpStatus const& 							getStatus() const;
		std::map<std::string, std::string> const&	getHeaders() const;
		std::string const&							getBody() const;

		class RawException: public std::runtime_error {
			public:
				RawException(std::string const& msg);
		};
};

std::ostream& operator<<(std::ostream& os, Response const& response);

#endif