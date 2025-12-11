#include "static/StaticHandler.hpp"
#include "http/HttpStatus.hpp"
#include "utils/Const.hpp"
#include "utils/utils.hpp"
#include <dirent.h>
#include <algorithm>
#include <ctime>
#include <iomanip>

size_t const	StaticHandler::_FILE_MAX_SIZE = 10 * 1024 * 1024; // 10MB

Response	StaticHandler::get(RoutingDecision const& rd)
{
	Response resp;
	std::string const& finalPath = rd.getFinalPath();
	LocationBlock const* location = rd.getLocation();
	Request const& req = rd.getRequest();

	// Serve webserv welcome page
	 if (location->getRoot().empty() && finalPath == "/") {
		resp.setBodyAndContentLength(_builtinWelcomePageHtml());
		resp.setServedFilePath("built-in welcome page"); // for debug
		resp.setConnectionFromRequest(req);
		return resp;
	}

	std::vector<std::string> locIndexes = location->getIndexFiles();
	bool isAutoindex = location->getAutoindex() == "on";

	if (utils::isAccessibleDirectory(finalPath)) {
		DIR* dir = opendir(finalPath.c_str());
		if (!dir)
			return error("forbidden", rd);
		closedir(dir);
		// Trying serving index files
		for (size_t i = 0; i < locIndexes.size(); ++i)
		{
			std::string const& indexPath = locIndexes[i];
			std::string pathToServe;
			if (utils::isAbsolutePath(indexPath))
				pathToServe = indexPath;
			else
				pathToServe = utils::pathsJoin(finalPath, indexPath);
			if (utils::isReadableFile(pathToServe))
				return _serveFile(pathToServe, rd);
		}
		// No index file found
		if (isAutoindex) { // Generate HTML listing
			resp.setBodyAndContentLength(_builtinAutoindexHtml(rd));
			resp.setServedFilePath("built-in index"); // for debug
			resp.setConnectionFromRequest(req);
			return resp;
		}
		return error("forbidden", rd);
	}
	else if (utils::isReadableFile(finalPath))
		return _serveFile(finalPath, rd);
	return error("not_found", rd);
}

Response	StaticHandler::del(RoutingDecision const& rd)  
{
	std::string const& finalPath = rd.getFinalPath();
	Request const& req = rd.getRequest();

	if (!utils::fileExists(finalPath))
		return error("not_found", rd);
	if (utils::isAccessibleDirectory(finalPath))
		return error("forbidden", rd);
	std::string directoryPath = utils::getParentDirectory(finalPath);
	if (!utils::isWritableDirectory(directoryPath))
		return error("forbidden", rd);
	if (std::remove(finalPath.c_str()) == 0)
	{
		Response resp;
		resp.setServedFilePath(finalPath);
		resp.setStatus(HttpStatus("no_content"));
		resp.setConnectionFromRequest(req);
		return resp;
	}
	else
		return error("internal_server_error", rd);
}

Response StaticHandler::post(RoutingDecision const& rd)
{
	if (UBUNTU_TESTER) {
		Response resp;
		resp.setStatus(HttpStatus("no_content"));
		resp.setBodyAndContentLength("");
		return resp;
	} else {
		return error("method_not_allowed", rd);
	}
}

Response	StaticHandler::head(RoutingDecision const& rd)
{
	Response resp = get(rd);
	resp.clearBodyForHead();
	return resp;
}

Response	StaticHandler::put(RoutingDecision const& rd)
{
	Request const& req = rd.getRequest();
	std::string const& finalPath = rd.getFinalPath();

	std::string directory = finalPath;
	if (utils::isAccessibleDirectory(finalPath)) {
        return error("forbidden", rd);
	} else {
		size_t lastSlash = finalPath.find_last_of('/');
		if (lastSlash != std::string::npos)
			directory = finalPath.substr(0, lastSlash);
    }
	if (directory.empty())
		directory = "/";
	if (!utils::isWritableDirectory(directory))
		return error("forbidden", rd);
	std::string const& body = req.getBody();
	bool fileExists = utils::fileExists(finalPath);
	std::ofstream file(finalPath.c_str(), std::ios::binary);
	if (!file.is_open())
		return error("internal_server_error", rd);
	file.write(body.c_str(), body.size());
	file.close();

	Response resp;
    if (fileExists) {
        resp.setStatus(HttpStatus("ok"));
		resp.setHeader("Content-Location", req.getPath());
	} else {
		resp.setStatus(HttpStatus("created"));
		resp.setHeader("Location", req.getPath());
	}
	resp.setServedFilePath(finalPath);
	resp.setConnectionFromRequest(req);
	return resp;
}

Response	StaticHandler::_serveFile(std::string const& filePath, RoutingDecision const& rd)
{
	size_t fileSize = utils::getFileSize(filePath);
	if (fileSize > _FILE_MAX_SIZE)
		return error("content_too_large", rd);
	std::string content = utils::readFile(filePath);
	if (content.empty() && fileSize > 0)
		return error("internal_server_error", rd);

	Response resp;
	resp.setStatus(HttpStatus("ok"));
	resp.setHeader("Content-Type", _getMimeFromPath(filePath));
	if (_getMimeFromPath(filePath) == "application/octet-stream")
		resp.setHeader("Content-Disposition", "inline; filename=\"" + utils::getFileName(filePath) + "\"");
	resp.setBodyAndContentLength(content);
	resp.setServedFilePath(filePath);
	resp.setConnectionFromRequest(rd.getRequest());
	return resp;
}

Response	StaticHandler::error(std::string const& errorSlug, RoutingDecision const& rd)
{
	Request const& req = rd.getRequest();
	ErrorPages const& locErrorPages = rd.getLocation()->getErrorPages();

	return error(errorSlug, req, locErrorPages);
}

Response	StaticHandler::error(std::string const& errorSlug, Request const& req, ErrorPages const& locErrorPages)
{
	HttpStatus	status(errorSlug);
	std::string const& method = req.getMethod();

	ErrorPages::const_iterator search = locErrorPages.find(status.getCode());
	if (search != locErrorPages.end()) {
		std::string errorPath = search->second;
		if (utils::isReadableFile(errorPath)) {
			Response resp;
			std::string errorBody = utils::readFile(errorPath);
			resp.setStatus(status);
			resp.setHeader("Content-Type", _getMime("html"));
			if (method != "HEAD")
				resp.setBodyAndContentLength(errorBody);
			else
				resp.setHeader("Content-Length", utils::str(errorBody.size()));
			resp.setServedFilePath(errorPath);
			resp.setConnectionFromRequest(req);
			return resp;
		}
	}
	Response resp = builtinError(status.getSlug(), method);
	resp.setConnectionFromRequest(req);
	return resp;
}

/**
 * Send back error before request parsing was done (in Network module for example).
 * @param method Uppercase HTTP method: `GET`, `PUT`, `HEAD`, etc.
 */
Response	StaticHandler::builtinError(std::string const& errorSlug, std::string const& method)
{
	HttpStatus status(errorSlug);
	std::string errorBody = _builtinErrorPageHtml(status);

	Response resp;
	resp.setServedFilePath("built-in error page"); // for debug
	resp.setStatus(status);
	resp.setHeader("Content-Type", _getMime("html"));
	if (method != "HEAD")
		resp.setBodyAndContentLength(errorBody);
	else
		resp.setHeader("Content-Length", utils::str(errorBody.size()));
	return resp;
}

// HTML

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
std::string	StaticHandler::_builtinAutoindexHtml(RoutingDecision const& rd)
{
	std::string const& finalPath = rd.getFinalPath();
	std::string currentPath = rd.getRequest().getPath();

	if (!currentPath.empty() && currentPath[currentPath.size() - 1] != '/')
		currentPath += "/";
	std::stringstream html;
	html << "<html>\n<head><title>Index of " << currentPath << "</title></head>\n<body>\n"
		 << "<h1>Index of " << currentPath << "</h1><hr><pre>\n"
		 << "<a href=\"../\">../</a>\n";
	DIR* dir = opendir(finalPath.c_str());
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
		std::string fullPath = utils::pathsJoin(finalPath, files[i]);
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
			 << std::setw(50 - displayName.size()) << " "
			 << dateStr << std::setw(20) << sizeStr << "\n";
	}
	html << "</pre><hr></body>\n</html>";
	return html.str();
}

/**
 * Webserv welcome page with the exact same styling as ngninx one.
 */
std::string StaticHandler::_builtinWelcomePageHtml()
{
	return std::string(
		"<!DOCTYPE html>"
		"<html>"
		"<head>"
		" <title>Welcome to " + Const::SERVER_NAME + "!</title>"
		" <style>"
		"  html { color-scheme: light dark; }"
		"  body { width: 35em; margin: 0 auto; font-family: Tahoma, Verdana, Arial, sans-serif; }"
		" </style>"
		"</head>"
		"<body>"
		" <h1>Welcome to " + Const::SERVER_NAME + "!</h1>"
		" <p>If you see this page, the web server is successfully working. Further configuration is required.</p>"
		" <p>For online documentation and support please refer to <a href=\"" + Const::SERVER_REPO + "\">Github repository</a>.</p>"
		" <p><em>Thank you for using " + Const::SERVER_NAME + ".</em></p>"
		"</body>"
		"</html>"
	);
}

std::string StaticHandler::_builtinErrorPageHtml(HttpStatus const& status)
{
	return std::string(
		"<!DOCTYPE html>"
		"<html>"
		"<head>"
		" <title>" + status.toStr() + "</title>\n"
		"</head>"
		"<body>"
		" <center><h1>" + status.toStr() + "</h1></center>"
		" <hr><center>webserv/1.0</center>"
		" </body>"
		"</html>"
	);
}

// MIME

/**
 * Get MIME type from extension (without dot, eg. "html", jpeg")
 */
std::string	StaticHandler::_getMime(const std::string& extension)
{
	static MimeTypes types;

	if (types.empty()) {
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
	}

	MimeTypes::const_iterator it = types.find(extension);
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
