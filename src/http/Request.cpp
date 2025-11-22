#include "http/RequestParser.hpp"

std::set<std::string>	Request::_supportedMethods;
std::set<std::string>	Request::_existingMethods;

Request::Request()
: _status(HttpStatus())
{}

Request::Request(std::string const& rawRequest)
: _status(HttpStatus())
{
	RequestParser::parseRequest(*this, rawRequest);
}

Request::Request(const Request& other)
: _status(other._status)
, _method(other._method)
, _uri(other._uri)
, _path(other._path)
, _scriptName(other._scriptName)
, _pathInfo(other._pathInfo)
, _queryString(other._queryString)
, _version(other._version)
, _headers(other._headers)
, _contentType(other._contentType)
, _body(other._body)
, _rawRequest(other._rawRequest)
{}

Request& Request::operator=(const Request& other)
{
	if (this != &other)
	{
		_status = other._status;
		_method = other._method;
		_uri = other._uri;
		_path = other._path;
		_scriptName = other._scriptName;
		_pathInfo = other._pathInfo;
		_queryString = other._queryString;
		_version = other._version;
		_headers = other._headers;
		_contentType = other._contentType;
		_body = other._body;
		_rawRequest = other._rawRequest;
	}
	return (*this);
}

Request::~Request()
{}

std::set<std::string> const&	Request::getSupportedMethods()
{
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

bool Request::isSupportedMethod(const std::string& method)
{
    getSupportedMethods();
    return _supportedMethods.find(method) != _supportedMethods.end();
}

bool	Request::isExistingMethod(std::string const& method)
{
	if (_existingMethods.empty()) {
		_existingMethods.insert("OPTIONS");
		_existingMethods.insert("PATCH");
		_existingMethods.insert("CONNECT");
		_existingMethods.insert("TRACE");
	}
	return _existingMethods.find(method) != _existingMethods.end();
}

bool	Request::isConnectionClose() const {
	for (size_t i = 0; i < _headers.size(); ++i) {
		std::string name = utils::toLowerCase(_headers[i].first);
		std::string value = utils::toLowerCase(utils::trim(_headers[i].second));
		if (name == "connection" && value == "close")
			return true;
	}
	return false;
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

const std::map<std::string, std::string> Request::getHeaders() const
{
	return getCombinedHeaders();
}

const std::vector<std::pair<std::string, std::string> >& Request::getAllHeaders() const
{
	return _headers;
}

std::map<std::string, std::string> Request::getCombinedHeaders() const
{
	std::map<std::string, std::string> combined;

	for (std::vector<std::pair<std::string, std::string> >::const_iterator it = _headers.begin();
		it != _headers.end(); ++it)
	{
		std::string normalizedName = utils::toLowerCase(it->first);
		if (combined.find(normalizedName) != combined.end())
			combined[normalizedName] += ", " + it->second;
		else
			combined[normalizedName] = it->second;
	}
	return combined;
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
	_headers.push_back(std::make_pair(name, value));
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
	const std::vector<std::pair<std::string, std::string> >& rawHeaders = request.getAllHeaders();
	os << "- Raw Headers: " << rawHeaders.size() << "\n";
	for (std::vector<std::pair<std::string, std::string> >::const_iterator it = rawHeaders.begin();
		it != rawHeaders.end(); ++it)
		os << "  - " << it->first << ": " << PrintableString(it->second) << "\n";
	const std::map<std::string, std::string>& combinedHeaders = request.getHeaders();
	os << "- Combined Headers: " << combinedHeaders.size() << "\n";
	for (std::map<std::string, std::string>::const_iterator it = combinedHeaders.begin();
		it != combinedHeaders.end(); ++it)
		os << "  - " << it->first << ": " << PrintableString(it->second) << "\n";
	os << "- Body: " << PrintableString(Log::excerpt(Log::EXCERPT_CHARS, request.getBody())) << "\n";
	os << "- Body Length: " << request.getBody().length() << "\n";
	os << "- raw request: " << PrintableString(Log::excerpt(Log::EXCERPT_CHARS, request.getRawRequest())) << "\n";
    return os;
}
