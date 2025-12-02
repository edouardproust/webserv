#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

class Network;
#include "cgi/CgiContext.hpp"
#include "cgi/SafePipe.hpp"
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

	void	_setupChildProcess(SafePipe&, SafePipe&, SafePipe&, CgiData const&);
	void	_setupParentProcess(SafePipe&, SafePipe&, SafePipe&, int, pid_t, CgiData const&);
    void	_writeRequestBody(int, Request const&);
    void	_registerPipesToEpoll(int, int);
    void	_createCgiContext(int, pid_t, int, int, Request const&); // TODO passer CgiData plutot que Request ?
	void	_finalizeResponse(CgiContext*, int);

	void	_errorFromContext(CgiContext*, std::string const&, std::string const&);
	void	_cleanupProcessRessources(pid_t);
	void	_killAndCleanupProcess(pid_t);
	void	_reapZombies();

	void	_sendNotImplementedResponse(int, CgiData const&);
	void	_sendPlaceholderResponse(int);

	// TODO canonical ? + comment
	CgiHandler();
	CgiHandler(CgiHandler const&);
	CgiHandler&	operator=(CgiHandler const&);

	public:

		CgiHandler(Network*);
		~CgiHandler();

		void	launchAsync(int, Response const&);
		void	readAndAccumulateCgiOutput(int);
		void	checkCompletion();
		void	errorFromClientFd(int, std::string const&, Request const&, ErrorPages const&, std::string const&);
		void	errorFromPipeFd(int, std::string const&, std::string const&);
		void	fullCleanup();

		bool	isCgiPipe(int) const;
		std::map<pid_t, CgiContext*> const&	getContextsByPid() const;
};

#endif