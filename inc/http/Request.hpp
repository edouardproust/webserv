#ifndef REQUEST_HPP
# define REQUEST_HPP

# include "constants.hpp"
# include <string>
# include <map>
# include <set>

class Request
{
	private :

	static std::set<std::string> _supportedMethods;
	ParseStatus	_status;
	std::string	method;
	std::string	path;
	std::string	version;
	std::map<std::string, std::string> headers;
	std::string	body;
	// Extra fields for easier routing
	std::string	host; // extracted from "Host" header
	int			port; // extracted from "Host" header
	std::string	queryString; // extracted from path if any
	std::string	contentType; // extracted from "Content-Type" header if any

	void	_extractHostAndPort(); //TODO
	void	_extractQueryString(); //TODO
	void	_extractContentType(); //TODO

	public:

	Request();
	Request(const Request& other);
	Request& operator=(const Request& other);
	~Request();

	void	parse(std::string const& rawRequest);

	static bool	isSupportedMethod(std::string const& method);

	ParseStatus const&	getStatus() const;
	std::string const&	getMethod() const;
	std::string	const&	getPath() const;
	std::string	const&	getVersion() const;
	std::map<std::string, std::string> const&	getHeaders() const;
	std::string const&	getBody() const;
	std::string const&	getHost() const;
	int					getPort() const;
	std::string const&	getQueryString() const;
	std::string const&	getContentType() const;

	void	setStatus(ParseStatus status);
	void	setMethod(std::string const& method);
    void	setPath(std::string const& path);
    void	setVersion(std::string const& version);
    void	addHeader(std::string const& name, std::string const& value);
	void	setBody(std::string const& body);
};

#endif
