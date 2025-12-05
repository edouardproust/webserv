#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

class Network;
#include "cgi/CgiContext.hpp"
#include "http/Response.hpp"
#include <sys/wait.h>

// TODO make canonical ? + class comment
class CgiHandler
{
	static size_t const	_TIMEOUT_SECONDS;
	static size_t const	_READ_BUFFER_SIZE;

	Network*						_network;
	std::map<int, CgiContext*>		_contextsByPipeFd;
	std::map<pid_t, CgiContext*>	_contextsByPid;
	std::set<pid_t>					_zombiesToReap;

	CgiContext*	_createCgiContext(int, CgiData const&);
	bool		_safeFork(CgiContext*, pid_t&);
	void		_executeChildProcess(CgiContext*, CgiData const&);
	void		_setupParentProcess(pid_t, CgiContext*);
	void		_setupStdinPipe(CgiContext*);
	void		_handleTimeout(CgiContext*, time_t);
	void		_cleanupPipes(CgiContext*);
	void		_closeStdinPipe(CgiContext*, int, std::string const&);
	void		_sendErrorResponse(std::string const&, int, CgiData const&, std::string const&, bool = false);

	// TODO canonical ? + comment
	CgiHandler();
	CgiHandler(CgiHandler const&);
	CgiHandler&	operator=(CgiHandler const&);

	public:

		CgiHandler(Network*);
		~CgiHandler();

		void	launchAsync(int, Response&);
		void	writeCgiInput(int);
		void	readCgiOutput(int);
		void	checkCompletion();
		bool	isCgiPipe(int) const;
		bool	hasActiveCgi(int) const;
		void	fullCleanup();

		std::map<pid_t, CgiContext*> const&	getContextsByPid() const;
		void	errorFromPipeFd(int, std::string const&, std::string const&);
};

#endif