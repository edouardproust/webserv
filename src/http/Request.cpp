#include "http/Request.hpp"
#include "http/RequestParser.hpp"

std::set<std::string> Request::_supportedMethods;

Request::Request() : _status(NOT_SET), _method(""), _requestTarget(""), _path(""), _queryString(""), _version(""), _contentType(""), _body("") {}

Request::Request(const Request& other) {
	*this = other;
}

Request& Request::operator=(const Request& other)
{
	if (this != &other)
	{
		this->_status = other._status;
		this->_method = other._method;
		this->_requestTarget = other._requestTarget;
		this->_path = other._path;
		this->_queryString = other._queryString;
		this->_version = other._version;
		this->_headers = other._headers;
		this->_contentType = other._contentType;
		this->_body = other._body;
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

const ParseStatus& Request::getStatus() const
{
	return (this->_status);
}

const std::string& Request::getMethod() const
{
	return this->_method;
}

const std::string& Request::getRequestTarget() const
{
	return this->_requestTarget;
}

const std::string& Request::getPath() const
{
	return this->_path;
}

const std::string& Request::getQueryString() const
{
	return this->_queryString;
}

const std::string& Request::getVersion() const
{
	return this->_version;
}

const std::map<std::string, std::string>& Request::getHeaders() const
{
	return this->_headers;
}

const std::string& Request::getContentType() const
{
    return _contentType;
}

const std::string& Request::getBody() const
{
	return this->_body;
}

void	Request::setStatus(ParseStatus status)
{
	this->_status = status;
}

void	Request::setMethod(std::string const& _method)
{
	this->_method = _method;
}

void	Request::setRequestTarget(const std::string& _requestTarget)
{
	this->_requestTarget = _requestTarget;
}

void	Request::setPath(const std::string& _path)
{
	this->_path = _path;
}

void	Request::setQueryString(const std::string& _queryString)
{
	this->_queryString = _queryString;
}

void	Request::setVersion(const std::string& _version)
{
	this->_version = _version;
}

void	Request::addHeader(const std::string& name, const std::string& value)
{
	_headers[name] = value;
}

void Request::setContentType(const std::string& value)
{
    _contentType = value;
}

void	Request::setBody(const std::string& _body)
{
	this->_body = _body;
}

std::ostream& operator<<(std::ostream& os, const Request& request)
{
	os << "Request:\n";
	ParseStatus const& status = request.getStatus();
	os << "- Status: ";
	if (status != NOT_SET) os << status << "\n";
	else os << "[empty]\n";
	std::string const& method = request.getMethod();
	os << "- Method: " << (!method.empty() ? method : "[empty]") << "\n";
	os << "- Request Target: " << request.getRequestTarget() << "\n";
	os << "- Path: " << request.getPath() << "\n";
	if (!request.getQueryString().empty())
		os << "- Query String: " << request.getQueryString() << "\n";
	os << "- Version: " << request.getVersion() << "\n";
	os << "- Headers: " << request.getHeaders().size() << "\n";
	const std::map<std::string, std::string>& headers = request.getHeaders();
	for (std::map<std::string, std::string>::const_iterator it = headers.begin();
		it != headers.end(); ++it)
		os << "  - " << it->first << ": " << it->second << "\n";
	os << "- Body: '" << request.getBody() << "'\n";
	os << "- Body Length: " << request.getBody().length() << "\n";
    return os;
}
