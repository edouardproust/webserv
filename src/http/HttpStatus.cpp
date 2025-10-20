#include "http/HttpStatus.hpp"

const HttpStatus::Entry HttpStatus::STATUS_TABLE[] = {
	{200, "OK", "ok"},
	{201, "Created", "created"},
	{204, "No Content", "no_content"},
	{301, "Moved Permanently", "moved_permanently"},
	{302, "Found", "found"},
	{400, "Bad Request", "bad_request"},
	{403, "Forbidden", "forbidden"},
	{404, "Not Found", "not_found"},
	{405, "Method Not Allowed", "method_not_allowed"},
	{411, "Length Required", "length_required"},
	{413, "Content Too Large", "content_too_large"},
	{500, "Internal Server Error", "internal_server_error"},
	{501, "Not Implemented", "not_implemented"},
	{502, "Bad Gateway", "bad_gateway"},
	{505, "HTTP Version Not Supported", "version_not_supported"},
};

const size_t HttpStatus::STATUS_TABLE_SIZE = sizeof(HttpStatus::STATUS_TABLE) / sizeof(HttpStatus::Entry);

HttpStatus::Entry const*	HttpStatus::_findByCode(int code) {
	for (size_t i = 0; i < STATUS_TABLE_SIZE; ++i)
		if (STATUS_TABLE[i].code == code)
			return &STATUS_TABLE[i];
	return NULL;
}

HttpStatus::Entry const* HttpStatus::_findBySlug(std::string const& slug) {
	for (size_t i = 0; i < STATUS_TABLE_SIZE; ++i)
		if (STATUS_TABLE[i].slug == slug)
			return &STATUS_TABLE[i];
	return NULL;
}

HttpStatus::HttpStatus(int code) {
	const Entry* found = _findByCode(code);
	if (found) {
		_code = found->code;
		_reason = found->reason;
		_slug = found->slug;
	} else {
		_code = 500;
		_reason = "Internal Server Error";
		_slug = "internal_server_error";
	}
}

HttpStatus::~HttpStatus() {}

HttpStatus::HttpStatus(std::string const& value) {
	// si valeur numérique => code
	if (!value.empty() && std::isdigit(value[0])) {
		int code = std::atoi(value.c_str());
		*this = HttpStatus(code);
	} else {
		*this = HttpStatus(value, true);
	}
}

HttpStatus::HttpStatus(std::string const& slug, bool isSlug) {
	const Entry* found = _findBySlug(slug);
	if (found) {
		_code = found->code;
		_reason = found->reason;
		_slug = found->slug;
	} else {
		_code = 500;
		_reason = "Internal Server Error";
		_slug = "internal_server_error";
	}
}

int	HttpStatus::getCode() const {
	return _code;
}
std::string const&	HttpStatus::getReason() const {
	return _reason;
}

std::string const&	HttpStatus::getSlug() const {
	return _slug;
}

/**
 * Returns the HttpStatus as a string, like "404 Not Found"
 */
std::string	HttpStatus::toString() const {
	std::ostringstream oss;
	oss << _code << " " << _reason;
	return oss.str();
}

HttpStatus	HttpStatus::fromSlug(std::string const& slug) {
	return HttpStatus(slug, true);
}

bool	HttpStatus::isError(int code) {
	return code >= 400;
}

std::ostream&	operator<<(std::ostream& os, const HttpStatus& rhs) {
	os << "HttpStatus: "
		<< "code=" << rhs.getCode()
		<< "reason=\"" << rhs.getReason() << "\""
		<< "slug=\"" << rhs.getSlug() << "\""
		<< "\n";
	return os;
}

