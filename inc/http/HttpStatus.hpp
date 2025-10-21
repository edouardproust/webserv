#ifndef HTTP_STATUS_HPP
#define HTTP_STATUS_HPP

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
	static Entry const*	_findBySlug(std::string const&);

	HttpStatus(); // Not used

	public:

		HttpStatus(int);
		HttpStatus(std::string const&);
		HttpStatus(HttpStatus const&);
		HttpStatus& operator=(HttpStatus const&);
		~HttpStatus();

		int					getCode() const;
		std::string const&	getReason() const;
		std::string const&	getSlug() const;

		std::string			toString() const;
		static bool			isError(int);

};

std::ostream&	operator<<(std::ostream&, const HttpStatus&);

#endif