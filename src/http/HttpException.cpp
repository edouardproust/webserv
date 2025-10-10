#include "http/HttpException.hpp"

HttpException::HttpException(int code, const std::string& message)
: _code(code), _message(message) {}

char const*	HttpException::what() const throw() {
	return _message.c_str();
}

int	HttpException::code() const {
	return _code;
}