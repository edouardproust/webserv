#ifndef REQUESTPARSER_HPP
# define REQUESTPARSER_HPP

# include "Request.hpp"
# include "HttpStatus.hpp"
# include "utils/Const.hpp"

/**
 * Parses raw HTTP requests into Request objects.
 *
 * Service-type class: not instantiable; all methods are static.
 */
class RequestParser
{
	static size_t const	_PATH_MAX_LEN;
	static size_t const	_HEADER_MAX_LEN;

	static HttpStatus	_parseRequestLine(Request&, std::string const&);
	static HttpStatus	_parseUri(Request&, std::string const&);
	static HttpStatus	_parseUrl(std::string&, std::string const&);
	static HttpStatus	_parseHeaders(Request&, std::string const&, bool);
	static HttpStatus	_parseHeaderLine(Request&, std::string const&);
	static HttpStatus	_parseChunkedBody(Request&);
	static HttpStatus	_validateBody(Request&);

	static bool			_isValidStart(std::string const&, size_t&);
	static bool			_isValidMethod(std::string const&);
	static bool			_isValidPath(std::string const&);
	static void			_extractScriptAndPathInfo(Request&, std::string const&);
	static bool			_isValidVersion(std::string const&);
	static bool			_isValidVersionNumber(std::string const&);
	static bool			_isValidHeaderName(std::string const&);
	static bool			_isValidHeaderValue(std::string const&);
	static bool			_headersIndicateBody(Request const&);
	static bool			_isValidContentType(std::string const&);
	static bool			_isValidContentLength(std::string const&);
	static bool			_hasBody(std::string const&, size_t);
	static std::string	_normalizeHeaderName(std::string const&);

	// Not instantiable
	RequestParser();
	RequestParser(RequestParser const&);
	RequestParser& operator=(RequestParser const&);
	~RequestParser();

	public :

		static void	parseRequest(Request&, std::string const&);
		static bool	isRawRequestComplete(std::string const&);

};

#endif
