#ifndef SAFE_PIPE_HPP
#define SAFE_PIPE_HPP

#include "utils/Log.hpp"
#include "utils/utils.hpp"
#include <fcntl.h>
#include <cstring>

// TODO verify canonical form + comment
/**
 * RAII wrapper for pipe file descriptors.
 * Manages both read and write ends with automatic cleanup.
 * Non-copyable and non-movable.
 */
class SafePipe
{
	private:
		int			_readFd;
		int			_writeFd;
		std::string	_description;

		void	_safeClose(int& fd, std::string const& end);
		void	_create();
		void	_closeAll();
		void	_setNonBlocking();

		// Non-copyable and non-movable
		SafePipe(SafePipe const&);
		SafePipe& operator=(SafePipe const&);

	public:
		SafePipe(std::string const& description);
		~SafePipe();

		void	closeRead();
		void	closeWrite();

		int					readFd() const;
		int					writeFd() const;
		const std::string&	getDescription() const;
};

#endif