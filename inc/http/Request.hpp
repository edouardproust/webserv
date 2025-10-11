#ifndef REQUEST_HPP
# define REQUEST_HPP

# include <string>
# include <map>
# include <set>

class Request
{
	private :

	static std::set<std::string> _supportedMethods;
	std::string	method;
	std::string	path;
	std::string	version;
	std::map<std::string, std::string> headers;
	std::string	body;

	public:

	Request();
	Request(const Request& other);
	Request& operator=(const Request& other);
	~Request();

	static bool	isSupportedMethod(std::string const& method);

	std::string const&	getMethod() const;
	std::string	const&	getPath() const;
	std::string	const&	getVersion() const;
	std::map<std::string, std::string> const&	getHeaders() const;
	std::string const&	getBody() const;

	void	setMethod(std::string const& method);
    void	setPath(std::string const& path);
    void	setVersion(std::string const& version);
    void	addHeader(std::string const& name, std::string const& value);
	void	setBody(std::string const& body);
};

#endif
