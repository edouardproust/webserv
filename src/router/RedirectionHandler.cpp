#include "router/RedirectionHandler.hpp"
#include "static/StaticHandler.hpp"
#include "utils/Const.hpp"

RedirectionHandler::RedirectionHandler(RoutingDecision const& rd, int code, std::string const& path)
: _routingDecision(rd)
, _code(code)
, _path(path)
{}

RedirectionHandler::~RedirectionHandler()
{}

/**
 * Executes the redirection by building an appropriate HTTP response.
 *
 * For 3xx redirects, sets the Location header and applies cache control for best pratice:
 * - 301 (Permanent): Cached for 1 year (max-age=31536000) since browsers cache these
 * - 302 (Temporary): Uses no-cache to ensure fresh redirects on subsequent requests
 *
 * Also generates a user-friendly HTML body with redirect information and links.
 * (This HTML will only be visible inside the browser if the client doesn't follow the redirect automatically)
 * Returns an internal server error if the redirect path is empty.
 */
Response	RedirectionHandler::run()
{
	if (_path.empty()) {
		return StaticHandler::error("internal_server_error", _routingDecision);
	}
	Response response;
	response.setServedFilePath("redirecting..."); // for debug
	response.setStatus(HttpStatus(_code));
	if (_code >= 300 && _code < 400)
		response.setHeader("Location", _path);
	if (_code == 301)
		response.setHeader("Cache-Control", "max-age=31536000");
	else if (_code == 302)
		response.setHeader("Cache-Control", "no-cache");
	response.setBodyAndContentLength(_redirectionHtml());
	response.setHeader("Content-Type", "text/html");
	return response;
}

void	RedirectionHandler::setCode(int code)
{
	_code = code;
}

void	RedirectionHandler::setPath(std::string const& path)
{
	_path = path;
}

int	RedirectionHandler::getCode() const
{
	return _code;
}

std::string const&	RedirectionHandler::getPath() const
{
	return _path;
}

std::string	RedirectionHandler::_redirectionHtml() const
{
	std::string html =
		"<!DOCTYPE html>"
		"<html>"
		"<head>"
		" <title>" + HttpStatus(_code).toStr() + "</title>"
		"</head>"
		"<body>"
		" <center><h1>" + HttpStatus(_code).toStr() + "</h1></center>";
	if	(_code >= 300 && _code < 400)
		html +=
			"<p>The document has moved <a href=\"" + _path + "\">here</a>.</p>"
			"<p>If you are not redirected automatically, follow the <a href=\"" + _path + "\">link</a>.</p>";
	html +=
		" <hr><center>" + Const::SERVER_SOFTWARE + "</center>"
		"</body>"
		"</html>";
	return html;
}

std::ostream& operator<<(std::ostream& os, RedirectionHandler const& rhs)
{
	os << "RedirectionHandler:\n";
	os << "- code: " << HttpStatus(rhs.getCode()) << "\n";
	os << "- path: " << PrintableString(rhs.getPath()) << "\n";
	os << "- type: " << (rhs.getCode() >= 300 && rhs.getCode() < 400 ? "HTTP Redirection" : "Return Code");
	return os;
}
