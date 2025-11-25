#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "cgi/CgiContext.hpp"
#include "http/Response.hpp"

// TODO make canonical ? + class comment
class CgiHandler
{
	void _finalize(CgiContext* ctx, int status);
	void _cleanup(CgiContext* ctx);

	int _epollFd;
	std::map<int, CgiContext*>		_contextsByPipe;
	std::map<pid_t, CgiContext*>	_contextsByPid;

	public:

		CgiHandler();
		~CgiHandler();

		void	init(int);
		void	launchAsync(int, Response const&,  std::map<int, std::string>&, std::map<int, size_t>&, std::map<int, bool>&);
		void	handlePipeRead(int);
		void	checkCompletion();
		void	cleanupAll();

		bool	isCgiPipe(int) const;
};

#endif