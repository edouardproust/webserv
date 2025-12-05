#ifndef HTTP_STATUS_HPP
#define HTTP_STATUS_HPP

#include "utils/utils.hpp"
#include "utils/Log.hpp"
#include <string>
#include <sstream>

/**
 * Represents an HTTP status code.
 *
 * Encapsulates code, reason phrase, and slug.
 * Value-type class: copyable and assignable; follows othodox canonical form.
 */
class HttpStatus
{
	struct Entry {
		int code;
		std::string const reason;
		std::string const slug;
	};

	int			_code;		// eg. 404
	std::string _reason;	// eg. "Not Found"
	std::string _slug;		// eg. "not_found"

	static const Entry STATUS_TABLE[];
	static const size_t STATUS_TABLE_SIZE;

	static Entry const*	_findByCode(int code);
	static Entry const*	_findBySlug(std::string const&);
	void				_initFromEntry(const Entry* entry);

	public:

 		// Othodox canonical form (Value-type class)
		HttpStatus(); // Default: 200 OK
		HttpStatus(int);
		HttpStatus(std::string const&);
		HttpStatus(HttpStatus const&);
		HttpStatus& operator=(HttpStatus const&);
		~HttpStatus();

		int					getCode() const;
		std::string			getCodeStr() const;
		std::string const&	getReason() const;
		std::string const&	getSlug() const;

		std::string			toStr() const;
		static bool			isError(int);
		static bool			isRedirection(int);

		bool	isError() const;

};

std::ostream&	operator<<(std::ostream&, const HttpStatus&);

#endif