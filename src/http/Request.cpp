#include "http/Request.hpp"

std::set<std::string> Request::_supportedMethods;

Request::Request() : _method(""), _path(""), _version(""), _body("") {}

Request::Request(const Request& other) {
	*this = other;
}

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

bool	Request::isSupportedMethod(std::string const& method) {
	if (_supportedMethods.empty()) {
		_supportedMethods.insert("GET");
		_supportedMethods.insert("POST");
		_supportedMethods.insert("DELETE");
		// More supported methods can be added here
	}
	return _supportedMethods.find(method) != _supportedMethods.end();
}

std::string const& Request::getMethod() const
{
	return this->_method;
}

std::string const& Request::getPath() const
{
	return this->_path;
}

std::string const& Request::getVersion() const
{
	return this->_version;
}

std::map<std::string, std::string> const& Request::getHeaders() const
{
	return this->_headers;
}

std::string const& Request::getBody() const
{
	return this->_body;
}

void	Request::setMethod(std::string const& _method)
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
