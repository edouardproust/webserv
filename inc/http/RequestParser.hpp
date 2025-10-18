#ifndef REQUESTPARSER_HPP
# define REQUESTPARSER_HPP

# include "http/Request.hpp"
# include "constants.hpp"
# include <string>
# include <sstream>

class RequestParser
{
	private :

	ParseStatus	_parseRequestLine(Request& request, const std::string& line);
	void 		_parseRequestTarget(Request& request, const std::string& _requestTarget) const;
	ParseStatus	_parseHeaders(Request& request, const std::string& headersPart, bool hasBody);
	ParseStatus	_parseHeaderLine(Request& request, const std::string& line);
	ParseStatus	_validateBody(const Request& request);

	bool				_isValidStart(const std::string& rawRequest, size_t& requestStart) const;
	bool				_isValidMethod(const std::string& _method) const;
	bool				_isValidPath(const std::string& _path) const;
	bool				_isValidVersion(const std::string& _version) const;
	bool				_isValidHeaderName(const std::string& name) const;
	static std::string	_trimOWS(const std::string& str);
	bool				_hasBody(const std::string& rawRequest, size_t headersEnd) const;
	std::string			_normalizeHeaderName(const std::string& name) const;

	public :

	RequestParser();
	RequestParser(const RequestParser& other);
	RequestParser& operator=(const RequestParser& other);
	~RequestParser();

	void	parseRequest(Request& request, const std::string& rawRequest);
};

#endif
