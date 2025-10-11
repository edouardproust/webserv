#ifndef REQUEST_HPP
# define REQUEST_HPP

# include <string>
# include <map>

enum Method
{
	GET,
	POST,
	DELETE,
	UNKNOWN
};

class Request
{
	private :

	Method	_method;
	std::string	_path;
	std::string	_version;
	std::map<std::string, std::string> _headers;
	std::string	_body;

	public:

	Request();
	Request(const Request& other);
	Request& operator=(const Request& other);
	~Request();

	Method	getMethod() const;
	std::string	getPath() const;
	std::string	getVersion() const;
	const std::map<std::string, std::string>&	getHeaders() const;
	std::string getBody() const;

	void	setMethod(Method _method);
    void	setPath(const std::string& _path);
    void	setVersion(const std::string& _version);
    void	addHeader(const std::string& name, const std::string& value);
	void	setBody(const std::string& _body);
};

#endif
