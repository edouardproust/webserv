#include "cgi/CgiHandler.hpp"

size_t const	CgiHandler::_FILES_THRESHOLD = 10 * 1024 * 1024; // 10MB

CgiHandler::CgiHandler(Request const& req)
: _handler(NULL)
, _useFiles(req.getBody().size() > _FILES_THRESHOLD)
{
    if (_useFiles) {
        _handler = new FilesCgiHandler();
    } else {
        _handler = new PipesCgiHandler();
    }
}

CgiHandler::ExecException::ExecException(std::string const& msg)
: std::runtime_error(msg)
{}

CgiHandler::TimeoutException::TimeoutException(std::string const& msg)
: std::runtime_error(msg)
{}

Response	CgiHandler::run(Request const& req, LocationBlock const* loc, std::string const& scriptName, HostPortPair const& listeningOn) {
	return _handler->run(req, loc, scriptName, listeningOn);
}
