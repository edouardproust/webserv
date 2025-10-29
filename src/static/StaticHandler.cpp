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

Response	StaticHandler::_serveFile(std::string const& filePath)
{
	size_t fileSize = _getFileSize(filePath);
	if (fileSize > MAX_FILE_SIZE)
		return handleError(HttpStatus(413), "", ErrorPages());
	std::string content = _readFile(filePath);
	if (content.empty() && fileSize > 0)
		return handleError(HttpStatus(500), "", ErrorPages());
	std::string contentType = _getMimeType(filePath);
	Response response;
	response.setStatus(200);
	response.setHeader("Content-Type", contentType);
	response.setBody(content);
	return response;
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

/*
 * Else, build a standard error page with `HttpStatus->toString()`.
 */
Response	StaticHandler::handleError(HttpStatus const& status, std::string const& locRoot, ErrorPages const& locErrorPages)
{
	std::cout << "[DEBUG] StaticHandler:\n"
		<< "- action: Error\n"
		<< "- status: " << status.toString() << "\n"
		<< "- location root: " << locRoot << "\n"
		<< "- error pages: " << locErrorPages.size() << "\n";
	for (ErrorPages::const_iterator it = locErrorPages.begin(); it != locErrorPages.end(); ++it)
		std::cout << "  - " << it->first << " -> " << it->second << "\n";
	std::cout << std::endl;

	// If one of `locErrorPages` corresponds to `HttpStatus->getCode()`, get its content.
	ErrorPages::const_iterator search = locErrorPages.find(status.getCode());
	if (search != locErrorPages.end())
	{
		std::string errorPath = utils::joinPath(locRoot, search->second);
		// if errorPath exists and is a readable file: return a response with its content
		// else return a Reponse with content of a built error page (not_found or forbidden)
		if (utils::isReadableFile(errorPath))
		{
			std::string content = _readFile(errorPath);
			Response response;
			response.setStatus(status);
			response.setBody(content);
			response.setHeader("Content-Type", "text/html");
			return response;
		}
	}
	// return a Response with the content of a built error page
	Response response;
	response.setStatus(status);
	std::string errorPage = _generateErrorPage(status.getCode(), status.getReason());
	response.setBody(errorPage);
	response.setHeader("Content-Type", "text/html");
	return response;
}

Response	StaticHandler::handleGet(Request const& request, std::string const& path, bool isAutoindex, std::vector<std::string> const& locIndexes)
{
	std::cout << "[DEBUG] StaticHandler:\n"
		<< "- handle: Get\n"
		<< "- method: " << request.getMethod() << "\n"
		<< "- file path: " << path << "\n"
		<< "- autoindex: " << (isAutoindex ? "true" : "false") << "\n"
		<< "- location indexes: " << locIndexes.size() << "\n";
	for (size_t i = 0; i < locIndexes.size(); ++i)
		std::cout << "  - " << locIndexes[i] << "\n";
	std::cout << std::endl;
	/*
	if path is a directory:
		if isAutoIndex: loop over indexes and build indexPath = utils::joinPath(path, indexes[i]). If a indexPath is a readable file: return a Response with its content
		else: return a Reponse with the content of a built directory index page
	else if is a regular file:
		if is an existing and readable file: return a Response with its content
		else: return a Response with a built error page (not_found or forbidden)
	*/
	if (utils::isAccessibleDirectory(path))
	{
		if (isAutoindex)
		{
			for (size_t i = 0; i < locIndexes.size(); ++i)
			{
				std::string indexPath = utils::joinPath(path, locIndexes[i]);
				if (utils::isReadableFile(indexPath))
				return _serveFile(indexPath);
			}
			return handleError(HttpStatus(404), path, ErrorPages());
		}	
		else
			return handleError(HttpStatus(404), path, ErrorPages());
	}
	else if (utils::isReadableFile(path))
		return _serveFile(path);
	else
		return handleError(HttpStatus(404), path, ErrorPages());
}

Response	StaticHandler::handleDelete(Request const& request, std::string const& path)
{
	(void)request;
	std::cout << "[DEBUG] StaticHandler:\n"
		<< "- handle: Delete\n"
		<< "- method: " << request.getMethod() << "\n"
		<< "- file path: " << path << "\n"
		<< std::endl;
	if (!utils::fileExists(path))
		return handleError(HttpStatus(404), "", ErrorPages());
	std::string dir = path.substr(0, path.find_last_of('/'));
	if (access(dir.c_str(), W_OK) != 0)
		return handleError(HttpStatus(403), "", ErrorPages());
	if (std::remove(path.c_str()) == 0)
	{
		Response response;
		response.setStatus(204);
		return response;
	}
	else
		return handleError(HttpStatus(500), "", ErrorPages());
}
