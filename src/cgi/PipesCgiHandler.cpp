#include "cgi/PipesCgiHandler.hpp"

PipesCgiHandler::PipesCgiHandler() {}

PipesCgiHandler::~PipesCgiHandler() {}

Response PipesCgiHandler::run(Request const& req, LocationBlock const* loc, std::string const& scriptName, HostPortPair const& listeningOn) {
	return Response(); //TODO
	(void)req, (void)loc, (void)scriptName, (void)listeningOn;
}
