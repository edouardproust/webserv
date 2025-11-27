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
	HttpStatus	_status;
	UniqHeaders	_headers;
	std::string	_body;
	bool		_bodyClearedForHead;

	void		_updateContentLength();
	void		_manageContentType();
	std::string	_buildStatusLine() const;
	std::string	_buildHeaders() const;
	void		_initDefaultHeaders();
	void		_parseRawResponse(std::string const&);
	int			_setHeaders(std::string const&);
	bool		_hasHeader(std::string const&) const;

	public:

 		// Othodox canonical form
		Response();
		Response(std::string const&);
		Response(Response const&);
		Response& operator=(Response const&);
		~Response();

		std::string stringify() const;

		void	setStatus(HttpStatus const&);
		void	setContentType(std::string const&);
		void	setHeader(std::string const&, std::string const&);
		void	setBody(std::string const&);
		void	clearBody();
		void	clearBodyForHead();
		void	setConnectionFromRequest(Request const&);
		bool	isConnectionClose() const;

		HttpStatus const& 	getStatus() const;
		UniqHeaders const&	getHeaders() const;
		std::string const&	getBody() const;

		class RawException: public std::runtime_error {
			public:
				RawException(std::string const&);
		};
};

std::ostream& operator<<(std::ostream&, Response const&);

#endif