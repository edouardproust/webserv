#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "http/HttpStatus.hpp"
#include "utils/utils.hpp"
#include "utils/Log.hpp"
#include "utils/typedefs.hpp"
#include <iostream>
#include <vector>
#include <map>

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
	AllHeaders	_allHeaders;
	UniqHeaders	_uniqHeaders;
	std::string	_contentType;
	std::string	_body;
	std::string	_rawRequest;
	std::map<std::string, std::string> _cookies;

	size_t		_loggedBytes;
	bool		_rawRequestComplete;
	time_t		_creationTime;

	static std::set<std::string> _supportedMethods;
	static std::set<std::string> _existingMethods;

	public:

 		// Othodox canonical form
		Request();
		Request(Request const&);
		Request& operator=(Request const&);
		~Request();

		void		parse();
		static std::set<std::string>	const& getSupportedMethods();
		static bool	isSupportedMethod(std::string const&);
		static bool	isExistingMethod(std::string const&);
		bool		isConnectionClose() const;
		void		rawRequestAppend(const char*, size_t);

		HttpStatus const&	getStatus() const;
		std::string const&	getMethod() const;
		std::string const&	getUri() const;
		std::string const&	getPath() const;
		std::string const&	getScriptName() const;
		std::string const&	getPathInfo() const;
		std::string const&	getQueryString() const;
		std::string const&	getVersion() const;
		AllHeaders const& 	getAllHeaders() const;
		UniqHeaders const&	getUniqHeaders() const;
		std::string const&	getContentType() const;
		std::string const&	getBody() const;
		std::string const&	getRawRequest() const;
		std::string 		getCookie(std::string const&) const;
		std::map<std::string, std::string> const&	getCookies() const;
		size_t				getLoggedBytes() const;
		bool				isRawRequestComplete() const;
		time_t				getCreationTime() const;

		void	setStatus(HttpStatus const&);
		void	setMethod(std::string const&);
		void	setUri(std::string const&);
		void	setPath(std::string const&);
		void	setScriptName(std::string const&);
		void	setPathInfo(std::string const&);
		void	setQueryString(std::string const&);
		void	setVersion(std::string const&);
		void	setHeader(std::string const&, std::string const&);
		void	setContentType(std::string const&);
		void	setBody(std::string const&);
		void	setRawRequest(std::string const&);
		void	addCookie(std::string const&, std::string const&);
		bool	hasCookie(std::string const&) const;
		void	setLoggedBytes(size_t);
		void	setRawRequestComplete(bool);
};

std::ostream&	operator<<(std::ostream&, Request const&);

#endif
