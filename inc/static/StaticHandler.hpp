#ifndef STATICHANDLER_HPP
#define STATICHANDLER_HPP

#include "http/Response.hpp"
#include "config/LocationBlock.hpp"
#include "utils/utils.hpp"
#include "typedefs.hpp"

class StaticHandler
{
	private:

	static std::map<std::string, std::string> _mimeTypes;

	static std::map<std::string, std::string> _initMimeTypes();
	static std::string _getMimeType(const std::string& filePath);
	static Response _serveFile(const std::string& filePath, LocationBlock const*);
	static Response _generateAutoindex(std::string const& dirPath, LocationBlock const* loc);
	static std::string _generateErrorPage(HttpStatus const& status);

	// TODO make canonical
	StaticHandler();
	StaticHandler(const StaticHandler& other);
	~StaticHandler();
	StaticHandler&	operator=(StaticHandler const& other);

	public:

	static Response	handleGet(const std::string& path, LocationBlock const* loc);
	static Response	handleDelete(std::string const& path, LocationBlock const* loc);
	//static Response	handlePut(std::string const&, Request const&);
	static Response	handleError(HttpStatus const& status, LocationBlock const* loc);

};

#endif
