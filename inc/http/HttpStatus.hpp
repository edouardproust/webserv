#ifndef PARSE_STATUS_HPP
#define PARSE_STATUS_HPP

#include <string>
#include <sstream>

class HttpStatus {

	struct Entry { int code; char const* reason; char const* slug; };

	int			_code;		// eg. 404
	std::string _reason;	// eg. "Not Found"
	std::string _slug;		// eg. "not_found"

	static const Entry STATUS_TABLE[];
	static const size_t STATUS_TABLE_SIZE;

	static Entry const*	_findByCode(int code);
	static Entry const*	_findBySlug(std::string const& slug);

	// Not used
	HttpStatus();
	HttpStatus(HttpStatus const&);
	HttpStatus(std::string const& slug, bool isSlug);
	HttpStatus& operator=(HttpStatus const&);

	public:

		HttpStatus(int code);
		HttpStatus(std::string const& str);
		~HttpStatus();

		int					getCode() const;
		std::string const&	getReason() const;
		std::string const&	getSlug() const;

		std::string			toString() const;
		static HttpStatus	fromSlug(std::string const& slug);
		static bool			isError(int code);

};

std::ostream&	operator<<(std::ostream&, const HttpStatus&);

#endif