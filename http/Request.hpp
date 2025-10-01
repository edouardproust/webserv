#ifndef REQUEST_HPP
# define REQUEST_HPP

# include <cstring>
# include <map.h>

enum class Method
{
	GET,
	POST,
	DELETE,
	UKNOWN
};

class Request
{
	private :

	std::string	method;
	std::string	path;
	std::string	version;
	std::map<std::string, std::string> headers;
	std::string	body;

	public:

	bool	request_parser(const std::string& rawRequest);
	std::string	getMethod() const;
	std::string	getPath() const;
	std::string	getVersion() const;
	std::string	getHeaders() const;
	std::string getBody() const;
};
