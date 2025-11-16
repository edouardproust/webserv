#include "static/StaticHandler.hpp"
#include "http/HttpStatus.hpp"
#include "constants.hpp"
#include "utils/utils.hpp"
#include <dirent.h>
#include <algorithm>
#include <ctime>
#include <iomanip>

StaticHandler::StaticHandler(RoutingDecision const& rd)
: _routingDecision(rd)
, _location(rd.getLocation())
, _finalPath(rd.getFinalPath())
{}

StaticHandler::~StaticHandler()
{}

Response	StaticHandler::handleGet()
{
	Response resp;

	// Serve webserv welcome page
	 if (_location->getRoot().empty() && _finalPath == "/") {
		resp.setBody(_welcomePageHtml());
		return resp;
	}

	std::vector<std::string> locIndexes = _location->getIndexFiles();
	bool isAutoindex = _location->getAutoindex() == "on";

	if (utils::isAccessibleDirectory(_finalPath))
	{
		DIR* dir = opendir(_finalPath.c_str());
		if (!dir)
			return handleError(HttpStatus("forbidden"));
		closedir(dir);
		// Trying serving index files
		for (size_t i = 0; i < locIndexes.size(); ++i)
		{
			std::string const& indexPath = locIndexes[i];
			std::string tmp;
			if (utils::isAbsolutePath(indexPath))
				tmp = indexPath;
			else {
				// no transversal check needed (finalPath already secured in RoutingDecision::_setFilePath)
				tmp = utils::pathsJoin(_finalPath, indexPath);
			}
			if (utils::isReadableFile(tmp)) {
				_finalPath = tmp; //!\ final path updated
				return _serveFile();
			}
		}
		// No index file found
		if (isAutoindex) { // Generate HTML listing
			resp.setBody(_autoindexHtml());
			return resp;

		}
		return handleError(HttpStatus("not_found"));
	}
	else if (utils::isReadableFile(_finalPath))
		return _serveFile();
	return handleError(HttpStatus("not_found"));
}

Response	StaticHandler::handleDelete()
{
	if (!utils::fileExists(_finalPath))
		return handleError(HttpStatus("not_found"));
	_finalPath = _finalPath.substr(0, _finalPath.find_last_of('/')); //!\ final path updated
	if (access(_finalPath.c_str(), W_OK) != 0)
		return handleError(HttpStatus("forbidden"));
	if (std::remove(_finalPath.c_str()) == 0)
	{
		Response response;
		response.setStatus(HttpStatus("no_content"));
		return response;
	}
	else
		return handleError(HttpStatus("internal_server_error"));
}

/**
 * Simply return an error (POST method not allowed on a static file).
 */
Response StaticHandler::handlePost() {
	return handleError(HttpStatus("method_not_allowed"));
}

Response	StaticHandler::handleHead()
{
	Response response = handleGet();
	response.clearBody();
	return response;
}

Response	StaticHandler::handlePut()
{
	Response response;
	Request const& request = _routingDecision.getRequest();

	std::string directory = _finalPath;
	if (utils::isAccessibleDirectory(_finalPath))
        return handleError(HttpStatus("forbidden"));
	else
	{
		size_t lastSlash = _finalPath.find_last_of('/');
		if (lastSlash != std::string::npos)
			directory = _finalPath.substr(0, lastSlash);
    }
	if (directory.empty())
		directory = "/";
	if (!utils::isWritableDirectory(directory))
		return handleError(HttpStatus("forbidden"));
	std::string const& body = request.getBody();
	bool fileExists = utils::fileExists(_finalPath);
	std::ofstream file(_finalPath.c_str(), std::ios::binary);
	if (!file.is_open())
		return handleError(HttpStatus("internal_server_error"));
	file.write(body.c_str(), body.size());
	file.close();
    if (fileExists)
	{
        response.setStatus(HttpStatus("ok"));
		response.setHeader("Content-Location", request.getPath());
	}
	else
	{
		response.setStatus(HttpStatus("created"));
		response.setHeader("Location", request.getPath());
	}
	return response;
}

Response	StaticHandler::handleError(HttpStatus const& status)
{
	std::string const& locRoot = _location->getRoot();
	ErrorPages const& locErrorPages = _location->getErrorPages();

	Response resp;
	resp.setStatus(status);
	resp.setContentType(_getMime("html"));

	ErrorPages::const_iterator search = locErrorPages.find(status.getCode());
	if (search != locErrorPages.end())
	{
		std::string errorPath = search->second;
		if (utils::isReadableFile(errorPath)) {
			_finalPath = errorPath; //!\ final path updated
			resp.setBody(utils::readFile(_finalPath));
			return resp;
		}
	}
	resp.setBody(_errorPageHtml(status));
	return resp;
}

// HTML

std::string StaticHandler::_welcomePageHtml() const
{
	std::string html;

	return std::string(
		"<!DOCTYPE html>"
		"<html>"
		"<head>"
		" <title>Welcome to " + SERVER_NAME + "!</title>"
		" <style>"
		"  html { color-scheme: light dark; }"
		"  body { width: 35em; margin: 0 auto; font-family: Tahoma, Verdana, Arial, sans-serif; }"
		" </style>"
		"</head>"
		"<body>"
		" <h1>Welcome to " + SERVER_NAME + "!</h1>"
		" <p>If you see this page, the web server is successfully working. Further configuration is required.</p>"
		" <p>For online documentation and support please refer to <a href=\"" + SERVER_REPO + "\">Github repository</a>.</p>"
		" <p><em>Thank you for using " + SERVER_NAME + ".</em></p>"
		"</body>"
		"</html>"
	);
}

/**
 * Generates an automatic directory listing HTML page.
 *
 * Creates a user-friendly file browser showing all files and subdirectories in the current directory.
 * For each entry, displays:
 * - Clickable filename/link (with "/" suffix for directories)
 * - Last modification date
 * - File size (or "-" for directories)
 *
 * The layout uses fixed-width spacing (50 characters for names) for aligned columns,
 * though exact alignment may vary depending on font and browser rendering.
 * Entries are sorted alphabetically for consistent browsing.
 */
std::string	StaticHandler::_autoindexHtml() const
{
	std::string currentPath = _routingDecision.getRequest().getPath();
	if (!currentPath.empty() && currentPath[currentPath.length() - 1] != '/')
		currentPath += "/";
	std::stringstream html;
	html << "<html>\n<head><title>Index of " << currentPath << "</title></head>\n<body>\n"
		 << "<h1>Index of " << currentPath << "</h1><hr><pre>\n"
		 << "<a href=\"../\">../</a>\n";
	DIR* dir = opendir(_finalPath.c_str());
	std::vector<std::string> files;
	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL)
	{
		std::string name = entry->d_name;
		if (name == "." || name == "..")
			continue;
		files.push_back(name);
	}
	closedir(dir);
	std::sort(files.begin(), files.end());
	for (size_t i = 0; i < files.size(); i++)
	{
		std::string fullPath = utils::pathsJoin(_finalPath, files[i]);
		bool isDir = utils::isAccessibleDirectory(fullPath);
		size_t fileSize = utils::getFileSize(fullPath);
		std::string sizeStr = isDir ? "-" : utils::str(fileSize);
		struct stat fileStat;
		std::string dateStr = "?";
		if (stat(fullPath.c_str(), &fileStat) == 0)
			dateStr = utils::formatDate(fileStat.st_mtime, "%d-%b-%Y %H:%M");
		std::string displayName = files[i] + (isDir ? "/" : "");
		html << "<a href=\"" << currentPath << files[i] << (isDir ? "/" : "") << "\">"
			 << displayName << "</a>"
			 << std::setw(50 - displayName.length()) << " "
			 << dateStr << std::setw(20) << sizeStr << "\n";
	}
	html << "</pre><hr></body>\n</html>";
	return html.str();
}

std::string StaticHandler::_getCurrentDateLocal(time_t time) const
{
	struct tm* timeinfo = localtime(&time);
	char buffer[20];
	strftime(buffer, sizeof(buffer), "%d-%b-%Y %H:%M", timeinfo);
	return buffer;
}

std::string StaticHandler::_errorPageHtml(HttpStatus const& status) const
{
	return std::string(
		"<!DOCTYPE html>"
		"<html>"
		"<head>"
		" <title>" + status.toStr() + "</title>\n"
		"</head>\n"
		"<body>\n"
		" <center><h1>" + status.toStr() + "</h1></center>\n"
		" <hr><center>webserv/1.0</center>\n"
		" </body>\n"
		"</html>"
	);
}

// MIME

/**
 * Get MIME type from extension (without dot, eg. "html", jpeg")
 */
std::string	StaticHandler::_getMime(const std::string& extension)
{
	static std::map<std::string, std::string> types;

	types["html"]	= "text/html";
	types["htm"]	= "text/html";
	types["css"]	= "text/css";
	types["txt"]	= "text/plain";

	types["jpg"]	= "image/jpeg";
	types["jpeg"]	= "image/jpeg";
	types["png"]	= "image/png";
	types["gif"]	= "image/gif";
	types["ico"]	= "image/x-icon";

	types["js"]		= "application/javascript";
	types["json"]	= "application/json";
	types["pdf"]	= "application/pdf";

	types["ttf"]	= "font/ttf";
	types["otf"]	= "font/otf";
	types["woff"]	= "font/woff";
	types["woff2"]	= "font/woff2";

	// -- additional types can be added here --

	std::map<std::string, std::string>::const_iterator it = types.find(extension);
	if (it != types.end())
		return it->second;
	return "application/octet-stream";
}

/**
 * Get MIME type from an absolute file path.
 */
std::string	StaticHandler::_getMimeFromPath(const std::string& filePath)
{
	std::string extension = utils::getFileExtension(filePath);
	if (!extension.empty() && extension[0] == '.')
		extension = extension.substr(1);
	return _getMime(extension);
}

// UTILS

// TODO (optionnal?): If is CGI file, call CGIHandler instead
Response	StaticHandler::_serveFile()
{
	size_t fileSize = utils::getFileSize(_finalPath);
	if (fileSize > MAX_FILE_SIZE)
		return handleError(HttpStatus("content_too_large"));
	std::string content = utils::readFile(_finalPath);
	if (content.empty() && fileSize > 0)
		return handleError(HttpStatus("internal_server_error"));

	Response resp;
	resp.setStatus(HttpStatus("ok"));
	resp.setContentType(_getMimeFromPath(_finalPath));
	resp.setBody(content);
	return resp;
}

std::string const&	StaticHandler::getFinalPath() const
{
	return _finalPath;
}

bool	StaticHandler::hasUpdatedFinalPath() const {
	return (_routingDecision.getFinalPath() != _finalPath);
}

std::ostream& operator<<(std::ostream& os, StaticHandler const& rhs) {
	os << "- finalPath: " << PrintableString(rhs.getFinalPath()) << "\n";

	return os;
}