#ifndef STATICHANDLER_HPP
#define STATICHANDLER_HPP

#include "http/Request.hpp"
#include "http/Response.hpp"
#include "http/HttpStatus.hpp"
#include "utils/utils.hpp"
#include <string>
#include <map>

class StaticHandler
{
	private:

	static std::map<std::string, std::string> _mimeTypes;

	static std::map<std::string, std::string> _initMimeTypes();
	static std::string 	_getMimeType(const std::string& filePath);
	static Response		_serveFile(const std::string& filePath);
	static std::string	_generateErrorPage(int statusCode, const std::string& reasonPhrase);

	public:

	//not used
	StaticHandler();
	StaticHandler(StaticHandler const&);
	~StaticHandler();
	StaticHandler&	operator=(StaticHandler const&);

	static Response	handleGet(Request const&, std::string const&, bool, std::vector<std::string> const&);
	static Response	handleDelete(Request const&, std::string const&);
	//static Response	handlePut(std::string const&, Request const&);
	static Response	handleError(HttpStatus const&, std::string const&, ErrorPages const&);

};

#endif
