#ifndef FILE_CGI_HANDLER_HPP
#define FILE_CGI_HANDLER_HPP

#include "cgi/ICgiHandler.hpp"

class FileCgiHandler : public ICgiHandler {

	public:

		// TODO canonical form
		FileCgiHandler();
		~FileCgiHandler();

		Response run(Request const&, LocationBlock const*, std::string const&, HostPortPair const&);

};

#endif