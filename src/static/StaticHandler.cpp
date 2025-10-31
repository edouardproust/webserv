#include "static/StaticHandler.hpp"
#include "http/HttpStatus.hpp"
#include "constants.hpp"
#include "utils/utils.hpp"

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

Response	StaticHandler::_serveFile(std::string const& filePath, LocationBlock const* loc)
{
	size_t fileSize = utils::getFileSize(filePath);
	if (fileSize > MAX_FILE_SIZE)
		return handleError(HttpStatus("content_too_large"), loc);
	std::string content = utils::readFile(filePath);
	if (content.empty() && fileSize > 0)
		return handleError(HttpStatus("internal_server_error"), loc);
	std::string contentType = _getMimeType(filePath);
	Response response;
	response.setStatus(HttpStatus("ok"));
	response.setHeader("Content-Type", contentType);
	response.setBody(content);
	return response;
}

std::string StaticHandler::_generateErrorPage(HttpStatus const& status)
{
	std::stringstream html;

	html << "<html>\n"
		 << "  <head>\n"
		 << "    <title>" << status.toString() << "</title>\n"
		 << "  </head>\n"
		 << "  <body>\n"
		 << "    <center><h1>" << status.toString() << "</h1></center>\n"
		 << "    <hr><center>webserv/1.0</center>\n"
		 << "  </body>\n"
		 << "</html>";
	return html.str();
}

Response	StaticHandler::handleError(HttpStatus const& status, LocationBlock const* loc)
{
	std::string const& locRoot = loc->getRoot();
	ErrorPages const& locErrorPages = loc->getErrorPages();

	ErrorPages::const_iterator search = locErrorPages.find(status.getCode());
	if (search != locErrorPages.end())
	{
		std::string errorPath = utils::joinPath(locRoot, search->second);
		if (utils::isReadableFile(errorPath))
		{
			std::string content = utils::readFile(errorPath);
			Response response;
			response.setStatus(status);
			response.setBody(content);
			response.setHeader("Content-Type", "text/html");
			return response;
		}
	}
	Response response;
	response.setStatus(status);
	std::string errorPage = _generateErrorPage(status);
	response.setBody(errorPage);
	response.setHeader("Content-Type", "text/html");
	return response;
}

Response	StaticHandler::handleGet(std::string const& path, LocationBlock const* loc)
{
	std::vector<std::string> locIndexes = loc->getIndexFiles();
	bool isAutoindex = loc->getAutoindex() == "on";

	if (utils::isAccessibleDirectory(path))
	{
		if (isAutoindex)
		{
			for (size_t i = 0; i < locIndexes.size(); ++i)
			{
				std::string indexPath = utils::joinPath(path, locIndexes[i]);
				if (utils::isReadableFile(indexPath))
				return _serveFile(indexPath, loc);
			}
			return handleError(HttpStatus("not_found"), loc);
		}
		else
			return handleError(HttpStatus("not_found"), loc);
	}
	else if (utils::isReadableFile(path))
		return _serveFile(path, loc);
	else
		return handleError(HttpStatus("not_found"), loc);
}

Response	StaticHandler::handleDelete(std::string const& path, LocationBlock const* loc)
{
	if (!utils::fileExists(path))
		return handleError(HttpStatus("not_found"), loc);
	std::string dir = path.substr(0, path.find_last_of('/'));
	if (access(dir.c_str(), W_OK) != 0)
		return handleError(HttpStatus("forbidden"), loc);
	if (std::remove(path.c_str()) == 0) // TODO Method std::remove is allowed ?
	{
		Response response;
		response.setStatus(HttpStatus("no_content"));
		return response;
	}
	else
		return handleError(HttpStatus("internal_server_error"), loc);
}
