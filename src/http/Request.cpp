#include "http/Request.hpp"

Request::Request() : method(UNKNOWN), path(""), version(""), body("") {}

Request::Request(const Request& other)
	:  method(other.method),
		path(other.path),
		version(other.version),
		headers(other.headers),
		body(other.body)
{}

Request& Request::operator=(const Request& other)
{
	if (this != &other)
	{
		this->method = other.method;
		this->path = other.path;
		this->version = other.version;
		this->headers = other.headers;
		this->body = other.body;
	}
	return (*this);
}

Request::~Request() {}

Method Request::getMethod() const
{
	return (this->method);
}

std::string Request::getPath() const
{
	return (this->path);
}

std::string Request::getVersion() const
{
	return (this->version);
}

const std::map<std::string, std::string>& Request::getHeaders() const
{
	return (this->headers);
}

std::string Request::getBody() const
{
	return (this->body);
}

void	Request::setMethod(Method method)
{
	this->method = method;
}

void	Request::setPath(const std::string& path)
{
	this->path = path;
}

void	Request::setVersion(const std::string& version)
{
	this->version = version;
}

void	Request::addHeader(const std::string& name, const std::string& value)
{
	headers[name] = value;
}

void	Request::setBody(const std::string& body)
{
	this->body = body;
}
