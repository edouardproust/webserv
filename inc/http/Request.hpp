#ifndef REQUEST_HPP
# define REQUEST_HPP

# include "constants.hpp"
# include <string>
# include <iostream>
# include <map>
# include <set>

class Request
{
	private :

		HttpStatus	_status;
		static std::set<std::string> _supportedMethods;
		std::string	_method;
		std::string	_requestTarget;
		std::string _path;
		std::string	_queryString;
		std::string	_version;
		std::map<std::string, std::string> _headers;
		std::string	_contentType;
		std::string	_body;

		// Not used
		Request();

	public:

		Request(std::string const& rawRequest);
		Request(const Request& other);
		Request& operator=(const Request& other);
		~Request();

		static bool	isSupportedMethod(std::string const& method);

		const HttpStatus&	getStatus() const;
		const std::string&	getMethod() const;
		const std::string&	getRequestTarget() const;
		const std::string&	getPath() const;
		const std::string&	getQueryString() const;
		const std::string&	getVersion() const;
		const std::map<std::string, std::string>&	getHeaders() const;
		const std::string&	getContentType() const;
		const std::string&	getBody() const;

		void	setStatus(HttpStatus status);
		void	setMethod(std::string const& method);
		void	setRequestTarget(std::string const& requestTarget);
		void	setPath(std::string const& path);
		void	setQueryString(std::string const& queryString);
		void	setVersion(std::string const& version);
		void	addHeader(std::string const& name, std::string const& value);
		void	setContentType(const std::string& value);
		void	setBody(std::string const& body);
};

std::ostream&	operator<<(std::ostream& os, const Request& request);

#endif
