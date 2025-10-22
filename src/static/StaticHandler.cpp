#include "static/StaticHandler.hpp"
#include "http/HttpStatus.hpp"
#include "constants.hpp"
#include "utils/utils.hpp"
#include <fstream>
#include <sstream>
#include <cctype>

std::map<std::string, std::string> StaticHandler::_mimeTypes = StaticHandler::_initMimeTypes();

std::map<std::string, std::string> StaticHandler::_initMimeTypes()
{
	std::map<std::string, std::string> types;
	types["html"] = "text/html";
	types["css"] = "text/css";
	types["txt"] = "text/plain";
	types["jpg"] = "image/jpeg";
	types["jpeg"] = "image/jpeg";
	types["png"] = "image/png";
	types["gif"] = "image/gif";
	types["ico"] = "image/x-icon";
	types["js"] = "application/javascript";
	types["json"] = "application/json";
	types["pdf"] = "application/pdf";
	return types;
}

std::string	StaticHandler::_getMimeType(const std::string& filePath)
{
	std::string extension = utils::getFileExtension(filePath);
	if (!extension.empty() && extension[0] == '.')
		extension = extension.substr(1);
	std::map<std::string, std::string>::const_iterator it = _mimeTypes.find(extension);
	if (it != _mimeTypes.end())
		return it->second;
	 return "application/octet-stream";
}

size_t	StaticHandler::_getFileSize(const std::string& path)
{
	std::ifstream file(path.c_str(), std::ios::binary | std::ios::ate);
	if (!file.is_open())
		return 0;
	return file.tellg();
}

std::string	StaticHandler::_readFile(const std::string& path)
{
	std::ifstream file(path.c_str(), std::ios::binary);
	if (!file.is_open())
		return "";
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

std::string StaticHandler::_generateErrorPage(int statusCode, const std::string& reasonPhrase)
{
	std::stringstream html;

	html << "<html>\n"
		 << "  <head>\n"
		 << "    <title>" << statusCode << " " << reasonPhrase << "</title>\n"
		 << "  </head>\n"
		 << "  <body>\n"
		 << "    <center><h1>" << statusCode << " " << reasonPhrase << "</h1></center>\n"
		 << "    <hr><center>webserv/1.0</center>\n"
		 << "  </body>\n"
		 << "</html>";
	return html.str();
}

Response	StaticHandler::handleRequest(std::string const& filePath, Request const& request)
{
	(void)request;
	size_t fileSize = _getFileSize(filePath);
	if (fileSize > MAX_FILE_SIZE)
		return handleError(HttpStatus(413));
	std::string content = _readFile(filePath);
	if (content.empty() && fileSize > 0)
		return handleError(HttpStatus(500));
	std::string contentType = _getMimeType(filePath);
	Response response;
	response.setStatusCode(200);
	response.setHeader("Content-Type", contentType);
	response.setBody(content);
	return response;
}

Response	StaticHandler::handleError(const HttpStatus& status)
{
	Response response;
	response.setStatusCode(status.getCode());
	std::string errorPage = _generateErrorPage(status.getCode(), status.getReason());
	response.setBody(errorPage);
	response.setHeader("Content-Type", "text/html");
	return response;
}
