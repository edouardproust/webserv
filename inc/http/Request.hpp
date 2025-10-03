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

	Method	method;
	std::string	path;
	std::string	version;
	std::map<std::string, std::string> headers;
	std::string	body;

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
};

#endif
