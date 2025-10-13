#include "http/Request.hpp"
#include "http/RequestParser.hpp"

std::set<std::string> Request::_supportedMethods;

Request::Request() :
	method(""),
	path(""),
	version(""),
	body(""),
	host("localhost"),
	port(80),
	queryString(""),
	contentType("")
{}

Request::Request(const Request& other) {
	*this = other;
}

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

void	Request::parse(std::string const& rawRequest) {
	RequestParser parser;
	parser.parseRequest(*this, rawRequest);
}

bool	Request::isSupportedMethod(std::string const& method) {
	if (_supportedMethods.empty()) {
		_supportedMethods.insert("GET");
		_supportedMethods.insert("POST");
		_supportedMethods.insert("DELETE");
		// More supported methods can be added here
	}
	return _supportedMethods.find(method) != _supportedMethods.end();
}

ParseStatus const& Request::getStatus() const
{
	return (this->_status);
}

std::string const& Request::getMethod() const
{
	return (this->method);
}

std::string const& Request::getPath() const
{
	return (this->path);
}

std::string const& Request::getVersion() const
{
	return (this->version);
}

std::map<std::string, std::string> const& Request::getHeaders() const
{
	return (this->headers);
}

std::string const& Request::getBody() const
{
	return (this->body);
}

void	Request::setStatus(ParseStatus status)
{
	this->_status = status;
}

void	Request::setMethod(std::string const& method)
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

std::string const& Request::getHost() const {
	return host;
}

int Request::getPort() const {
	return port;
}

std::string const& Request::getQueryString() const {
	return queryString;
}

std::string const& Request::getContentType() const {
	return contentType;
}
