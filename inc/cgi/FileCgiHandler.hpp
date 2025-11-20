#ifndef FILE_CGI_HANDLER_HPP
#define FILE_CGI_HANDLER_HPP

#include "cgi/CgiHandler.hpp"
#include <cstring>

// TODO canonical form?
// TODO class desc
class FileCgiHandler : public CgiHandler
{
	static std::string const	_TMP_INPUT_FILE;
	static std::string const	_TMP_OUTPUT_FILE;

	int		_stderrPipe[2];

	void	_prepareIo(std::string const& method, std::string const& reqBody);

	static int	_createEmptyTempFile(std::string const&, bool);
	static void	_writeBodyToTempFile(std::string const& reqBody);

	public:

		FileCgiHandler();
		~FileCgiHandler();

		Response execute(Request const&, LocationBlock const*, std::string const&, HostPortPair const&);
};

#endif