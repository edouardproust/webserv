#ifndef CGI_PARAMS_HPP
#define CGI_PARAMS_HPP

#include "config/LocationBlock.hpp"
#include "config/HostPortPair.hpp"

// TODO canonical class + class comment
class CgiParams
{
	bool		_isValid;

	std::string _scriptName;
	std::string _extension;
	std::string _executor;
	std::string _inputData;

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



	public:

		CgiParams();
		CgiParams(Request const&, LocationBlock const*, std::string const&, HostPortPair const&);
		CgiParams(CgiParams const&);
		CgiParams&	operator=(CgiParams const&);

		bool	isValid() const;

		std::string const&				getExecutor() const;
		std::string const&				getScriptName() const;
		std::string const&				getInputData() const;
		std::vector<std::string> const&	getEnvStorage() const;
		std::vector<char*> const&		getEnvp() const;
		std::vector<std::string> const&	getArgStorage() const;
		std::vector<char*> const&		getArgv() const;
		std::string const&				getExtension() const;
};

std::ostream&	operator<<(std::ostream&, CgiParams const&);

#endif