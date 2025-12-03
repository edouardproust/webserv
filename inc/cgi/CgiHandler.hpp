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

	void	_finalizeResponse(CgiContext*, int);

	void	_sendNotImplementedResponse(int, CgiData const&);
	void	_sendPlaceholderResponse(int);
	void	_startStreamingResponse(CgiContext*);
	void	_continueStreamingResponse(CgiContext*);
	void	_handleTimeout(CgiContext*, time_t);
	void	_cleanupPipes(CgiContext*);

	// TODO canonical ? + comment
	CgiHandler();
	CgiHandler(CgiHandler const&);
	CgiHandler&	operator=(CgiHandler const&);

	public:

		CgiHandler(Network*);
		~CgiHandler();

		void	launchAsync(int, Response const&);
		void	writeCgiInput(int);
		void	readCgiOutput(int);
		void	checkCompletion();
		bool	isCgiPipe(int) const;
		bool	hasActiveCgi(int) const;

		std::map<pid_t, CgiContext*> const&	getContextsByPid() const;

		void	errorFromPipeFd(int, std::string const&, std::string const&);
};

#endif