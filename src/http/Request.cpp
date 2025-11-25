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
, _allHeaders(other._allHeaders)
, _uniqHeaders(other._uniqHeaders)
, _contentType(other._contentType)
, _body(other._body)
, _rawRequest(other._rawRequest)
{}

Request& Request::operator=(const Request& other)
{
	if (this != &other) {
		_status = other._status;
		_method = other._method;
		_uri = other._uri;
		_path = other._path;
		_scriptName = other._scriptName;
		_pathInfo = other._pathInfo;
		_queryString = other._queryString;
		_version = other._version;
		_allHeaders = other._allHeaders;
		_uniqHeaders = other._uniqHeaders;
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
	for (size_t i = 0; i < _allHeaders.size(); ++i) {
		std::string name = utils::toLowerCase(_allHeaders[i].first);
		std::string value = utils::toLowerCase(utils::trim(_allHeaders[i].second));
		if (name == "connection" && value == "close")
			return true;
	}
	return false;
}

HttpStatus const& Request::getStatus() const
{
	return _status;
}

std::string const& Request::getMethod() const
{
	return _method;
}

std::string const& Request::getUri() const
{
	return _uri;
}

std::string const& Request::getPath() const
{
	static const std::string root("/");
	return _path.empty() ? root : _path;
}

std::string const& Request::getScriptName() const
{
	return _scriptName;
}

std::string const& Request::getPathInfo() const
{
	return _pathInfo;
}

std::string const& Request::getQueryString() const
{
	return _queryString;
}

std::string const& Request::getVersion() const
{
	return _version;
}

UniqHeaders const& Request::getUniqHeaders() const
{
	return _uniqHeaders;
}

AllHeaders const& Request::getAllHeaders() const
{
	return _allHeaders;
}

std::string const& Request::getContentType() const
{
    return _contentType;
}

std::string const& Request::getBody() const
{
	return _body;
}

std::string const&	Request::getRawRequest() const {
	return _rawRequest;
}

void	Request::setStatus(HttpStatus const& status)
{
	_status = status;
}

void	Request::setMethod(std::string const& method)
{
	_method = method;
}

void	Request::setUri(std::string const& uri)
{
	_uri = uri;
}

void	Request::setScriptName(std::string const& scriptName)
{
	_scriptName = scriptName;
}

void	Request::setPathInfo(std::string const& pathInfo)
{
	_pathInfo = pathInfo;
}

void	Request::setPath(std::string const& path)
{
	_path = path;
}

void	Request::setQueryString(std::string const& queryString)
{
	_queryString = queryString;
}

void	Request::setVersion(std::string const& version)
{
	_version = version;
}

void	Request::setHeader(std::string const& name, std::string const& value)
{
	// Update _allHeaders
	_allHeaders.push_back(std::make_pair(name, value));

	// Udpate _uniqHeaders
	std::string normalizedName = utils::toLowerCase(name);
	if (_uniqHeaders.find(normalizedName) != _uniqHeaders.end())
		_uniqHeaders[normalizedName] += ", " + value;
	else
		_uniqHeaders[normalizedName] = value;
}

void Request::setContentType(std::string const& value)
{
    _contentType = value;
}

void	Request::setBody(std::string const& body)
{
	_body = body;
}

void	Request::setRawRequest(std::string const& rawRequest) {
	_rawRequest = rawRequest;
}

std::ostream& operator<<(std::ostream& os, Request const& request)
{
	os << "- Status: " << request.getStatus() << "\n";
	os << "- Method: " << PrintableString(request.getMethod()) << "\n";
	os << "- URI: " << PrintableString(request.getUri()) << "\n";
	os << "  - Path: " << PrintableString(request.getPath()) << "\n";
	os << "  - Script Name: " << PrintableString(request.getScriptName()) << "\n";
	os << "  - Path Info: " << PrintableString(request.getPathInfo()) << "\n";
	os << "  - Query String: " << PrintableString(request.getQueryString()) << "\n";
	os << "- Version: " << PrintableString(request.getVersion()) << "\n";
	AllHeaders const& rawHeaders = request.getAllHeaders();
	os << "- Raw Headers: " << rawHeaders.size() << "\n";
	for (AllHeaders::const_iterator it = rawHeaders.begin();
		it != rawHeaders.end(); ++it)
		os << "  - " << it->first << ": " << PrintableString(it->second) << "\n";
	UniqHeaders const& combinedHeaders = request.getUniqHeaders();
	os << "- Combined Headers: " << combinedHeaders.size() << "\n";
	for (UniqHeaders::const_iterator it = combinedHeaders.begin();
		it != combinedHeaders.end(); ++it)
		os << "  - " << it->first << ": " << PrintableString(it->second) << "\n";
	os << "- Body: " << PrintableString(Log::excerpt(Log::EXCERPT_CHARS, request.getBody())) << "\n";
	os << "- Body Length: " << request.getBody().length() << "\n";
	os << "- raw request: " << PrintableString(Log::excerpt(Log::EXCERPT_CHARS, request.getRawRequest())) << "\n";
    return os;
}
