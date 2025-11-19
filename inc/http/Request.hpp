#ifndef REQUEST_HPP
# define REQUEST_HPP

# include "http/HttpStatus.hpp"
# include "utils/utils.hpp"
# include "utils/Log.hpp"
# include <iostream>
# include <vector>
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
	std::vector<std::pair<std::string, std::string> > _headers;
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

		const HttpStatus&	getStatus() const;
		const std::string&	getMethod() const;
		const std::string&	getUri() const;
		const std::string&	getPath() const;
		const std::string&	getScriptName() const;
		const std::string&	getPathInfo() const;
		const std::string&	getQueryString() const;
		const std::string&	getVersion() const;
		const std::map<std::string, std::string> getHeaders() const;
		const std::vector<std::pair<std::string, std::string> >& getAllHeaders() const;
		std::map<std::string, std::string> getCombinedHeaders() const;
		const std::string&	getContentType() const;
		const std::string&	getBody() const;
		const std::string&	getRawRequest() const;

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
