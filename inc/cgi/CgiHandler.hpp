#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include "network/Network.hpp"
#include "cgi/CgiContext.hpp"
#include "http/Response.hpp"
#include <sys/wait.h>

// TODO make canonical ? + class comment
class CgiHandler
{
	static size_t const	_TIMEOUT_SECONDS;

	Network*						_network;
	std::map<int, CgiContext*>		_contextsByPipeFd;
	std::map<pid_t, CgiContext*>	_contextsByPid;
	std::set<pid_t>					_zombiesToReap;

	void	_launchProcess(int clientFd, Response const& cgiResponse);
	void	_finalizeResponse(CgiContext* ctx, int exitStatus);
	void	_cleanupProcessRessources(pid_t);
	void	_killAndCleanupProcess(pid_t);
	void	_addPipeToEpoll(int fd, std::string const& context);
	void	_reapZombies();

	// TODO canonical ? + comment
	CgiHandler(CgiHandler const&);
	CgiHandler&	operator=(CgiHandler const&);

	public:

		CgiHandler();
		~CgiHandler();

		void	init(Network*);
		void	launchAsync(int, Response const&);
		void	handlePipeRead(int);
		void	checkCompletion();
		void	handleError(CgiContext*, std::string const&, std::string const&);
		void	fullCleanup();

		bool	isCgiPipe(int) const;
};

#endif