#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "cgi/ICgiHandler.hpp"

/**
 * Handles execution of CGI scripts.
 *
 * Resource-type RAII class: manages pipes and child processes, non-copyable.
 */
class CgiHandler
{
	static size_t const			_FILES_THRESHOLD;

	ICgiHandler*	_handler;
    bool			_useFiles;

	public:

		static Response execute(Request const&, LocationBlock const*, std::string const&, HostPortPair const&);

		class ExecException: public std::runtime_error {
			public:
				ExecException(std::string const&);
		};

		class TimeoutException: public std::runtime_error {
			public:
				TimeoutException(std::string const&);
		};

};

#endif