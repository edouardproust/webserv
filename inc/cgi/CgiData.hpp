#ifndef CGI_DATA_HPP
#define CGI_DATA_HPP

#include "config/LocationBlock.hpp"
#include "config/HostPortPair.hpp"

/**
 * Holds all data needed for CGI execution.
 *
 * Non-Value type class (it contains non-owning references: _request, _errorPages):
 * - Copy-constructible only (for ownership transfer)
 * - Not default-constructible (requires valid Request reference)
 * - Not assignable (references cannot be reassigned)
 *
 * Intended usage: Create once, transfer ownership via copy, never reassign.
 */
class CgiData
{
	bool	_isValid;

	std::string			_scriptName;
	std::string			_extension;
	std::string			_executor;
	Request const&		_request; // non-owning
	ErrorPages const&	_errorPages; // non-owning
	std::string			_remoteAddr;

	// Storage for execve (need to stay valid during execution)
	std::vector<std::string>	_envStorage;	// Persistant storage for environment strings
	std::vector<char*>			_envp;			// Array for execve
	std::vector<std::string>	_argStorage;	// Persistant storage to arguments
	std::vector<char*>			_argv;			// Array for execve

	void	_setEnvStorage(Request const&, std::string const&, HostPortPair const&);
	void	_setArgStorage();
	void	_setEnvp();
	void	_setArgv();

	static std::string	_headerToEnvVar(std::string const&);

	// Forbidden
	CgiData(); // Cannot init references
	CgiData&	operator=(CgiData const&); // Cannot reassign references

	public:

		CgiData(CgiData const&); // Allowed: for ownership transfer
		CgiData(Request const&, LocationBlock const&, std::string const&, HostPortPair const&, std::string const&);
		~CgiData();

		bool				isValid() const;
		std::string const&	getScriptName() const;
		std::string const&	getExtension() const;
		std::string const&	getExecutor() const;
		Request const&		getRequest() const;
		ErrorPages const&	getErrorPages() const;

		std::vector<std::string> const&	getEnvStorage() const;
		std::vector<char*> const&		getEnvp() const;
		std::vector<std::string> const&	getArgStorage() const;
		std::vector<char*> const&		getArgv() const;
};

std::ostream&	operator<<(std::ostream&, CgiData const&);

#endif