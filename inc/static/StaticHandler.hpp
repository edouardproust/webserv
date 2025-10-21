#ifndef STATICHANDLER_HPP
#define STATICHANDLER_HPP

#include "constants.hpp"
#include "http/HttpStatus.hpp"
#include "http/Request.hpp"
#include "http/Response.hpp"
#include <string>
#include <map>

class StaticHandler
{
	/*private:

	static std::map<std::string, std::string> _mimeTypes;

	static std::map<std::string, std::string> _initMimeTypes();
	static std::string _getMimeType(const std::string& filePath);
	static std::string _generateErrorPage(int statusCode, const std::string& reasonPhrase) const;*/

	public:

	// Not used
	StaticHandler();
	StaticHandler(StaticHandler const&);
	~StaticHandler();
	StaticHandler&	operator=(StaticHandler const&);

	static Response	handleRequest(std::string const&, Request const&);
	static Response	handleError(HttpStatus);

	//std::string getMimeType(const std::string& filePath) const;

};

#endif
