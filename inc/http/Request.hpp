#ifndef REQUEST_HPP
# define REQUEST_HPP

# include <string>
# include <iostream>
# include <map>
# include <set>

class Request
{
	private :

	static std::set<std::string> _supportedMethods;
	std::string	_method;
	std::string	_requestTarget;
	std::string _path;
	std::string	_queryString;
	std::string	_version;
	std::map<std::string, std::string> _headers;
	std::string	_body;

	public:

	Request();
	Request(const Request& other);
	Request& operator=(const Request& other);
	~Request();

	static bool	isSupportedMethod(std::string const& method);

	std::string const&	getMethod() const;
	std::string	const&	getRequestTarget() const;
	std::string	const&	getPath() const;
	std::string	const&	getVersion() const;
	std::map<std::string, std::string> const&	getHeaders() const;
	std::string const&	getBody() const;
	std::string const&	getQueryString() const;

	void	setMethod(std::string const& method);
    void	setRequestTarget(std::string const& requestTarget);
	void	setPath(std::string const& path);
	void	setQueryString(std::string const& queryString);
    void	setVersion(std::string const& version);
    void	addHeader(std::string const& name, std::string const& value);
	void	setBody(std::string const& body);
};

std::ostream&	operator<<(std::ostream& os, const Request& request);

#endif
