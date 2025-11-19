#include "cgi/FileCgiHandler.hpp"

FileCgiHandler::FileCgiHandler() {}

FileCgiHandler::~FileCgiHandler() {}

Response FileCgiHandler::run(Request const& req, LocationBlock const* loc, std::string const& scriptName, HostPortPair const& listeningOn) {
	return Response(); //TODO
	(void)req, (void)loc, (void)scriptName, (void)listeningOn;
}