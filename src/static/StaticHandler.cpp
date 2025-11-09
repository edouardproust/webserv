#include "static/StaticHandler.hpp"
#include "http/HttpStatus.hpp"
#include "constants.hpp"
#include "utils/utils.hpp"
#include <dirent.h>
#include <algorithm>
#include <ctime>

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

// TODO (optionnal?): If is CGI file, call CGIHandler instead
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

std::string StaticHandler::_getCurrentDateLocal(time_t time)
{
	struct tm* timeinfo = localtime(&time);
	char buffer[20];
	strftime(buffer, sizeof(buffer), "%d-%b-%Y %H:%M", timeinfo);
	return buffer;
}

Response	StaticHandler::_generateAutoindex(std::string const& dirPath, LocationBlock const* loc)
{
	(void)loc;
	std::stringstream html;
	html << "<html>\n<head><title>Index of " << dirPath << "</title></head>\n<body>\n"
		 << "<h1>Index of " << dirPath << "</h1><hr><pre>\n"
		 << "<a href=\"../\">../</a>\n";
	DIR* dir = opendir(dirPath.c_str());
	if (!dir)
		return handleError(HttpStatus("forbidden"), loc);
	std::string files[100];
	int fileCount = 0;
	struct dirent* entry; //required by readdir
	while ((entry = readdir(dir)) != NULL && fileCount < 100)
	{
		std::string name = entry->d_name;
		if (name == "." || name == "..")
			continue;
		files[fileCount++] = name;
	}
	closedir(dir);
	std::sort(files, files + fileCount);
	for (int i = 0; i < fileCount; i++)
	{
		std::string fullPath = utils::joinPath(dirPath, files[i]);
		bool isDir = utils::isAccessibleDirectory(fullPath);
		size_t fileSize = utils::getFileSize(fullPath);
		std::string sizeStr = isDir ? "-" : utils::toString(fileSize);
		struct stat fileStat;
		std::string dateStr = "?";
		if (stat(fullPath.c_str(), &fileStat) == 0)
			dateStr = _getCurrentDateLocal(fileStat.st_mtime);
		std::string displayName = files[i] + (isDir ? "/" : "");
		html << "<a href=\"" << files[i] << (isDir ? "/" : "") << "\">" 
			 << displayName << "</a>"
			 << std::string(50 - displayName.length(), ' ')
			 << files[i] << (isDir ? "/" : "") << "</a>\n"
			 << dateStr << "                  " << sizeStr << "\n";
	}
	html << "</pre><hr></body>\n</html>";
	Response response;
	response.setStatus(HttpStatus(200));
	response.setBody(html.str());
	return response;
}

std::string StaticHandler::generateStatusHtml(HttpStatus const& status)
{
	std::stringstream html;

	html << "<html>\n"
		 << "  <head>\n"
		 << "    <title>" << status.toString() << "</title>\n"
		 << "  </head>\n"
		 << "  <body>\n"
		 << "    <center><h1>" << status.toString() << "</h1></center>\n"
		 << "    <hr><center>" << SERVER_SOFTWARE << "</center>\n"
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
	std::string errorPage = generateStatusHtml(status);
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
		// Trying serving index files
		for (size_t i = 0; i < locIndexes.size(); ++i)
		{
			std::string indexPath = utils::joinPath(path, locIndexes[i]);
			if (utils::isReadableFile(indexPath))
			return _serveFile(indexPath, loc);
		}
		// No index file found
		if (isAutoindex)
			return _generateAutoindex(path, loc); // Generate HTML listing
		else
			return handleError(HttpStatus("forbidden"), loc);
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
	if (std::remove(path.c_str()) == 0)
	{
		Response response;
		response.setStatus(HttpStatus("no_content"));
		return response;
	}
	else
		return handleError(HttpStatus("internal_server_error"), loc);
}
