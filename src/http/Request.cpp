#include "http/Request.hpp"

Request::Request() : _method(UNKNOWN), _path(""), _version(""), _body("") {}

Request::Request(const Request& other)
	:  _method(other._method),
		_path(other._path),
		_version(other._version),
		_headers(other._headers),
		_body(other._body)
{}

Request& Request::operator=(const Request& other)
{
	if (this != &other)
	{
		this->_method = other._method;
		this->_path = other._path;
		this->_version = other._version;
		this->_headers = other._headers;
		this->_body = other._body;
	}
	return (*this);
}

Request::~Request() {}

Method Request::getMethod() const
{
	return this->_method;
}

std::string Request::getPath() const
{
	return this->_path;
}

std::string Request::getVersion() const
{
	return this->_version;
}

const std::map<std::string, std::string>& Request::getHeaders() const
{
	return this->_headers;
}

std::string Request::getBody() const
{
	return this->_body;
}

void	Request::setMethod(Method _method)
{
	this->_method = _method;
}

void	Request::setPath(const std::string& _path)
{
	this->_path = _path;
}

void	Request::setVersion(const std::string& _version)
{
	this->_version = _version;
}

void	Request::addHeader(const std::string& name, const std::string& value)
{
	_headers[name] = value;
}

void	Request::setBody(const std::string& _body)
{
	this->_body = _body;
}
