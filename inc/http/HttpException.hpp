#include <exception>
#include <string>

class HttpException : public std::exception {

    int			_code;
    std::string	_message;

	public:

		HttpException(int code, const std::string&);

		virtual char const*	what() const throw();

		int code() const;

};
