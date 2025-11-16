#include "router/RedirectionHandler.hpp"
#include "static/StaticHandler.hpp"
#include "constants.hpp"

RedirectionHandler::RedirectionHandler(RoutingDecision const& rd, int code, std::string const& path)
: _routingDecision(rd)
, _code(code)
, _path(path)
{}

RedirectionHandler::RedirectionHandler(RedirectionHandler const& other)
: _routingDecision(other._routingDecision)
, _code(other._code)
, _path(other._path)
{}

RedirectionHandler&	RedirectionHandler::operator=(RedirectionHandler const& other)
{
	if (this != &other)
	{
		_routingDecision = other._routingDecision;
		_code = other._code;
		_path = other._path;
	}
	return *this;
}

RedirectionHandler::~RedirectionHandler()
{}

int	RedirectionHandler::getCode() const
{
	return _code;
}

std::string const& RedirectionHandler::getPath() const
{
	return _path;
}

void RedirectionHandler::setCode(int code)
{
	_code = code;
}

void RedirectionHandler::setPath(std::string const& path)
{
	_path = path;
}

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
Response	RedirectionHandler::execute()
{
	if (_path.empty())
	{
		StaticHandler static_(_routingDecision);
		return static_.handleError(HttpStatus("internal_server_error"));
	}
	Response response;
	response.setStatus(HttpStatus(_code));
	if (_code >= 300 && _code < 400)
		response.setHeader("Location", _path);
	if (_code == 301)
		response.setHeader("Cache-Control", "max-age=31536000");
	else if (_code == 302)
		response.setHeader("Cache-Control", "no-cache");
	response.setBody(_generateRedirectionHtml());
	response.setContentType("text/html");
	return response;
}

std::string	RedirectionHandler::_generateRedirectionHtml() const
{
	std::stringstream html;

	html << "<!DOCTYPE html>"
		 << "<html>"
		 << "<head>"
		 << "<title>" << _code << " " << HttpStatus(_code).getReason() << "</title>"
		 << "</head>"
		 << "<body>"
		 << "<center><h1>" << _code << " " << HttpStatus(_code).getReason() << "</h1></center>";
	if (_code >= 300 && _code < 400)
	{
		html << "<p>The document has moved <a href=\"" << _path << "\">here</a>.</p>"
			 << "<p>If you are not redirected automatically, follow the <a href=\"" << _path << "\">link</a>.</p>";
	}
	html << "<hr><center>" << SERVER_SOFTWARE << "</center>"
		 << "</body>"
		 << "</html>";
	return html.str();
}

Response	RedirectionHandler::run(RoutingDecision const& rd, int code, std::string const& path)
{
	RedirectionHandler handler(rd, code, path);
	return handler.execute();
}

std::ostream& operator<<(std::ostream& os, RedirectionHandler const& rhs)
{
	os << "RedirectionHandler:\n"
		<< "- code: " << HttpStatus(rhs.getCode()) << "\n"
		<< "- path: " << PrintableString(rhs.getPath()) << "\n"
		<< "- type: " << (rhs.getCode() >= 300 && rhs.getCode() < 400 ? "HTTP Redirection" : "Return Code");
	return os;
}
