#ifndef CGI_CONTEXT_HPP
#define CGI_CONTEXT_HPP

#include "cgi/CgiData.hpp"

/**
 * Holds runtime state for a single CGI execution (PID, pipes, timestamps).
 *
 * Resource-like entity: uniquely represents a running CGI process and its
 * associated file descriptors. Copying would duplicate ownership of
 * process-related resources, which is unsafe. Therefore the class is
 * not default-constructible, not copyable, not assignable.
 *
 * The class stores a non-owning pointer to the originating Request.
 */
class CgiContext
{
	pid_t				_pid;
	int					_clientFd;
	CgiData const&		_cgiData; // non-owning
	Request	const&		_request; // non-owning
	ErrorPages const&	_errorPages; // non-owning
	int					_inReadFd;
	int					_inWriteFd;
	int					_outReadFd;
	int					_outWriteFd;
	int					_errReadFd;
	int					_errWriteFd;

	std::string			_output; // owning
	std::string			_error; // owning
	time_t				_startTime;
	size_t				_inputBytesSent;
	bool				_headersReceived;
	bool				_headersSent;


	static void	_safeCloseFd(int&);

	// Not default-constructible, not copyable, not assignable
	CgiContext();
	CgiContext(CgiContext const&);
	CgiContext& operator=(CgiContext const&);

	public:

		CgiContext(int, CgiData const&, int, int, int, int, int, int);
		~CgiContext();

		void	setPid(int);
		void	appendOutput(const char*, size_t);
    	void	appendError(const char*, size_t);
		void	setStartTime();
		void	addInputBytesSent(size_t);

		void	closeInReadFd();
		void	closeInWriteFd();
		void	closeOutReadFd();
		void	closeOutWriteFd();
		void	closeErrReadFd();
		void	closeErrWriteFd();
		void	closeAllFds();

		pid_t				getPid() const;
		int					getClientFd() const;
		CgiData const&		getCgiData() const;
		Request const&		getRequest() const;
		ErrorPages const&	getErrorPages() const;
		int					getInReadFd() const;
		int					getInWriteFd() const;
		int					getOutReadFd() const;
		int					getOutWriteFd() const;
		int					getErrReadFd() const;
		int					getErrWriteFd() const;
		std::string	const&	getOutput() const;
		std::string const&	getError() const;
		time_t const&		getStartTime() const;
		size_t				getInputBytesSent() const;
		bool				headersReceived() const;
		void				setHeadersReceived(bool);
		bool				headersSent() const;
		void				setHeadersSent(bool);
};

std::ostream&	operator<<(std::ostream&, CgiContext const&);

#endif