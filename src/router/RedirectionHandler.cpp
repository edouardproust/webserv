#include "router/RedirectionHandler.hpp"
#include "static/StaticHandler.hpp"
#include "constants.hpp"

RedirectionHandler::RedirectionHandler(RoutingDecision const& rd, int code, std::string const& path)
	: _routingDecision(rd), _code(code), _path(path) {}

RedirectionHandler::RedirectionHandler(RedirectionHandler const& other)
	: _routingDecision(other._routingDecision), _code(other._code), _path(other._path) {}

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

RedirectionHandler::~RedirectionHandler() {}

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
	html << "<hr><center>" << SERVER_NAME << SERVER_VERSION << "</center>"
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
		<< "- code: " << rhs.getCode() << " (" << HttpStatus(rhs.getCode()).getReason() << ")\n"
		<< "- path: " << (rhs.getPath().empty() ? "[empty]" : rhs.getPath()) << "\n"
		<< "- type: " << (rhs.getCode() >= 300 && rhs.getCode() < 400 ? "HTTP Redirection" : "Return Code");
	return os;
}
