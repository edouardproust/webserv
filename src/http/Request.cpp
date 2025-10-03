#include "Request.hpp"

Request::Request() : method(UNKNOWN), path(""), version(""), body("") {}

Request::Request(const Request& other)
	: method(other.method),
    path(other.path),
    version(other.version),
    headers(other.headers),
    body(other.body) {}

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
