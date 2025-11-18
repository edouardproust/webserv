#ifndef REQUEST_HPP
# define REQUEST_HPP

# include "http/HttpStatus.hpp"
# include "utils/utils.hpp"
# include "utils/Log.hpp"
# include <iostream>
# include <map>
# include <set>

/**
 * Represents an HTTP request, including method, URI, headers, body, and version.
 *
 * Value-type class: copyable and assignable; owns its parsed request data.
 */
class Request
{
	HttpStatus	_status;
	std::string	_method;
	std::string	_uri; // URI = path + query string
	std::string _path; // Path = script name + path info
	std::string _scriptName;
	std::string	_pathInfo;
	std::string	_queryString;
	std::string	_version;
	std::map<std::string, std::string> _headers;
	std::string	_contentType;
	std::string	_body;
	std::string	_rawRequest;

	static std::set<std::string> _supportedMethods;
	static std::set<std::string> _existingMethods;

	public:

 		// Othodox canonical form
		Request();
		Request(std::string  const& rawRequest);
		Request(Request const& other);
		Request& operator=(Request const& other);
		~Request();

		static std::set<std::string> const& getSupportedMethods();
		static bool	isSupportedMethod(std::string const& method);
		static bool	isExistingMethod(std::string const& method);
		bool		isConnectionClose() const;

		HttpStatus const&	getStatus() const;
		std::string const&	getMethod() const;
		std::string const&	getUri() const;
		std::string const&	getPath() const;
		std::string const&	getScriptName() const;
		std::string const&	getPathInfo() const;
		std::string const&	getQueryString() const;
		std::string const&	getVersion() const;
		std::map<std::string, std::string> const&	getHeaders() const;
		std::string const&	getContentType() const;
		std::string const&	getBody() const;
		std::string const&	getRawRequest() const;
		std::string			getHeaderValue(std::string const&) const;

		void	setStatus(HttpStatus const& status);
		void	setMethod(std::string const& method);
		void	setUri(std::string const& uri);
		void	setPath(std::string const& path);
		void	setScriptName(std::string const& scriptName);
		void	setPathInfo(std::string const& pathInfo);
		void	setQueryString(std::string const& queryString);
		void	setVersion(std::string const& version);
		void	addHeader(std::string const& name, std::string const& value);
		void	setContentType(const std::string& value);
		void	setBody(std::string const& body);
		void	setRawRequest(std::string const& rawRequest);
};

std::ostream&	operator<<(std::ostream& os, const Request& request);

#endif
