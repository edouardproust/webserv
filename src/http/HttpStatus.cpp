#include "http/HttpStatus.hpp"

const HttpStatus::Entry HttpStatus::STATUS_TABLE[] = {
	{200, "OK", "ok"},
	{201, "Created", "created"},
	{204, "No Content", "no_content"},
	{301, "Moved Permanently", "moved_permanently"},
	{302, "Found", "found"},
	{307, "Temporary Redirect", "temporary_redirect"},
	{308, "Permanent Redirect", "permanent_redirect"},
	{400, "Bad Request", "bad_request"},
	{403, "Forbidden", "forbidden"},
	{404, "Not Found", "not_found"},
	{405, "Method Not Allowed", "method_not_allowed"},
	{411, "Length Required", "length_required"},
	{413, "Content Too Large", "content_too_large"},
	{414, "URI Too Long", "uri_too_long"},
	{431, "Request Header Fields Too Large", "request_header_fields_too_large"},
	{500, "Internal Server Error", "internal_server_error"}, // default (do not delete!)
	{501, "Not Implemented", "not_implemented"},
	{502, "Bad Gateway", "bad_gateway"},
	{504, "Gateway Timeout", "gateway_timeout"},
	{505, "HTTP Version Not Supported", "version_not_supported"},
};

// constructors

/**
 * Build a `HttpStatus` of 200 OK (default).
 */
HttpStatus::HttpStatus()
{
	_initFromEntry(_findBySlug("ok"));
}

/**
 * Build a `HttpStatus` from a HTTP status code.
 *
 * - Supported codes: `200`, `201`, `204`, `301`, `302`, `307`, `308`,
 * `400`, `403`,`404`, `405`, `411`, `413`, `500`, `501`, `502`, `505`
 * - Any other code defaults to `500`
 */
HttpStatus::HttpStatus(int code)
{
	_initFromEntry(_findByCode(code));
}

/**
 * Build a `HttpStatus` from a HTTP status slug.
 *
 * - Supported slugs: `ok`, `created`, `no_content`, `moved_permanently`,
 * `found`, `bad_request`, `forbidden`, `not_found`, `method_not_allowed`,
 * `length_required`, `content_too_large`, `internal_server_error`,
 * `not_implemented`, `bad_gateway`, `version_not_supported`
 * - Any other slug will default to `internal_server_error`
 */
HttpStatus::HttpStatus(std::string const& slug)
{
	_initFromEntry(_findBySlug(slug));
}

HttpStatus::HttpStatus(HttpStatus const& other)
: _code(other._code)
, _reason(other._reason)
, _slug(other._slug)
{}

HttpStatus& HttpStatus::operator=(HttpStatus const& other)
{
	if (this != &other) {
		_code = other._code;
		_reason = other._reason;
		_slug = other._slug;
	}
	return *this;
}

HttpStatus::~HttpStatus()
{}

// public

int	HttpStatus::getCode() const
{
	return _code;
}

std::string	HttpStatus::getCodeStr() const
{
	return utils::str(_code);
}

std::string const&	HttpStatus::getReason() const
{
	return _reason;
}

std::string const&	HttpStatus::getSlug() const
{
	return _slug;
}

/**
 * Returns the HttpStatus as a string like "404 Not Found".
 */
std::string	HttpStatus::toStr() const
{
	std::ostringstream oss;
	oss << _code << " " << _reason;
	return oss.str();
}

bool	HttpStatus::isError() const
{
	return _code >= 400;
}

bool	HttpStatus::isRedirection(int code)
{
	return code >= 300 && code < 400 && code != 304;
}

std::ostream&	operator<<(std::ostream& os, const HttpStatus& rhs)
{
	os << "HttpStatus: {"
		<< rhs.getCode() << ", "
		<< PrintableString(rhs.getReason()) << ", "
		<< PrintableString(rhs.getSlug())
		<< "}";
	return os;
}

// private

const size_t HttpStatus::STATUS_TABLE_SIZE = sizeof(HttpStatus::STATUS_TABLE) / sizeof(HttpStatus::Entry);

HttpStatus::Entry const*	HttpStatus::_findByCode(int code)
{
	for (size_t i = 0; i < STATUS_TABLE_SIZE; ++i)
		if (STATUS_TABLE[i].code == code)
			return &STATUS_TABLE[i];
	return NULL;
}

HttpStatus::Entry const* HttpStatus::_findBySlug(std::string const& slug)
{
	for (size_t i = 0; i < STATUS_TABLE_SIZE; ++i)
		if (STATUS_TABLE[i].slug == slug)
			return &STATUS_TABLE[i];
	return NULL;
}

void	HttpStatus::_initFromEntry(const Entry* entry)
{
	if (!entry)
		entry = _findByCode(500); // fallback to 500 Internal Server Error
	_code = entry->code;
	_reason = entry->reason;
	_slug = entry->slug;
}
