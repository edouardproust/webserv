#include "http/Request.hpp"
#include "http/RequestParser.hpp"

std::set<std::string> Request::_supportedMethods;
std::set<std::string> Request::_existingMethods;

Request::Request()
: _status(HttpStatus("ok")),
  _method(""),
  _uri(""),
  _path(""),
  _scriptName(""),
  _pathInfo(""),
  _queryString(""),
  _version(""),
  _contentType(""),
  _body("")
{}

Request::Request(std::string const& rawRequest)
: _status(HttpStatus("ok")),
  _method(""),
  _uri(""),
  _path(""),
  _scriptName(""),
  _pathInfo(""),
  _queryString(""),
  _version(""),
  _contentType(""),
  _body("")
{
	static RequestParser parser;
	parser.parseRequest(*this, rawRequest);
}

Request::Request(const Request& other): _status(other._status) {
	*this = other;
}

Request& Request::operator=(const Request& other)
{
	if (this != &other)
	{
		this->_status = other._status;
		this->_method = other._method;
		this->_uri = other._uri;
		this->_path = other._path;
		this->_scriptName = other._scriptName;
		this->_pathInfo = other._pathInfo;
		this->_queryString = other._queryString;
		this->_version = other._version;
		this->_headers = other._headers;
		this->_contentType = other._contentType;
		this->_body = other._body;
	}
	return (*this);
}

Request::~Request() {}

std::set<std::string> const&	Request::getSupportedMethods() {
	if (_supportedMethods.empty()) {
		_supportedMethods.insert("GET");
		_supportedMethods.insert("POST");
		_supportedMethods.insert("DELETE");
		_supportedMethods.insert("HEAD");
		_supportedMethods.insert("PUT");
		// -- more supported methods can be added here --
	}
	return _supportedMethods;
}

bool Request::isSupportedMethod(const std::string& method) {
    getSupportedMethods();
    return _supportedMethods.find(method) != _supportedMethods.end();
}

bool	Request::isExistingMethod(std::string const& method) {
	if (_existingMethods.empty()) {
		_existingMethods.insert("OPTIONS");
		_existingMethods.insert("PATCH");
		_existingMethods.insert("CONNECT");
		_existingMethods.insert("TRACE");
	}
	return _existingMethods.find(method) != _existingMethods.end();
}

const HttpStatus& Request::getStatus() const
{
	return (this->_status);
}

const std::string& Request::getMethod() const
{
	return this->_method;
}

const std::string& Request::getUri() const
{
	return this->_uri;
}

const std::string& Request::getPath() const
{
	static const std::string root("/");
	return _path.empty() ? root : _path;
}

const std::string& Request::getScriptName() const
{
	return this->_scriptName;
}

const std::string& Request::getPathInfo() const
{
	return this->_pathInfo;
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
    return this->_contentType;
}

const std::string& Request::getBody() const
{
	return this->_body;
}

std::string const&	Request::getRawRequest() const {
	return _rawRequest;
}

void	Request::setStatus(HttpStatus const& status)
{
	this->_status = status;
}

void	Request::setMethod(std::string const& method)
{
	this->_method = method;
}

void	Request::setUri(const std::string& uri)
{
	this->_uri = uri;
}

void	Request::setScriptName(const std::string& scriptName)
{
	this->_scriptName = scriptName;
}

void	Request::setPathInfo(const std::string& pathInfo)
{
	this->_pathInfo = pathInfo;
}

void	Request::setPath(const std::string& path)
{
	this->_path = path;
}

void	Request::setQueryString(const std::string& queryString)
{
	this->_queryString = queryString;
}

void	Request::setVersion(const std::string& version)
{
	this->_version = version;
}

void	Request::addHeader(const std::string& name, const std::string& value)
{
	_headers[name] = value;
}

void Request::setContentType(const std::string& value)
{
    this->_contentType = value;
}

void	Request::setBody(const std::string& body)
{
	this->_body = body;
}

void	Request::setRawRequest(const std::string& rawRequest) {
	this->_rawRequest = rawRequest;
}

std::ostream& operator<<(std::ostream& os, const Request& request)
{
	os << "- Status: " << request.getStatus() << "\n";
	os << "- Method: " << PrintableString(request.getMethod()) << "\n";
	os << "- URI: " << PrintableString(request.getUri()) << "\n";
	os << "  - Path: " << PrintableString(request.getPath()) << "\n";
	os << "  - Script Name: " << PrintableString(request.getScriptName()) << "\n";
	os << "  - Path Info: " << PrintableString(request.getPathInfo()) << "\n";
	os << "  - Query String: " << PrintableString(request.getQueryString()) << "\n";
	os << "- Version: " << PrintableString(request.getVersion()) << "\n";
	os << "- Headers: " << request.getHeaders().size() << "\n";
	const std::map<std::string, std::string>& headers = request.getHeaders();
	for (std::map<std::string, std::string>::const_iterator it = headers.begin();
		it != headers.end(); ++it)
		os << "  - " << it->first << ": " << PrintableString(it->second) << "\n";
	os << "- Body: " << PrintableString(utils::excerpt(EXCERPT_LENGTH, request.getBody())) << "\n";
	os << "- Body Length: " << request.getBody().length() << "\n";
	os << "- raw request: " << PrintableString(request.getRawRequest()) << "\n";
    return os;
}
