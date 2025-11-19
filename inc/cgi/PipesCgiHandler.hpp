#ifndef PIPES_CGI_HANDLER_HPP
#define PIPES_CGI_HANDLER_HPP

#include "cgi/ICgiHandler.hpp"

class PipesCgiHandler : public ICgiHandler {

	public:

		// TODO canonical form
		PipesCgiHandler();
		~PipesCgiHandler();

		Response run(Request const&, LocationBlock const*, std::string const&, HostPortPair const&);

};

#endif