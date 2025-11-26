#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "network/Network.hpp"
#include "cgi/CgiContext.hpp"
#include "http/Response.hpp"
#include <sys/wait.h>

// TODO make canonical ? + class comment
class CgiHandler
{
	void	_launchProcess(int clientFd, Response const& cgiResponse);
	void	_finalizeResponse(CgiContext* ctx, int exitStatus);
	void	_cleanup(CgiContext* ctx); // CgiContexts are owned by Cgihandler
	void	_addPipeToEpoll(int fd, std::string const& context);

	Network*						_network;
	std::map<int, CgiContext*>		_contextsByPipeFd;
	std::map<pid_t, CgiContext*>	_contextsByPid;

	public:

		CgiHandler();
		~CgiHandler();

		void	init(Network*);
		void	launchAsync(int, Response const&);
		void	handlePipeRead(int);
		void	handlePipeError(int);
		void	checkCompletion();
		void	cleanupAll();

		bool	isCgiPipe(int) const;
};

#endif