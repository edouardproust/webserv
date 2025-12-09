#ifndef SESSION_HPP
# define SESSION_HPP

# include <string>
# include <ctime>
# include <map>
/**
 * Represents a single user session with identity and timing.
 * 
 * Each Session object contains:
 * - Private attributes: session ID (_sessionId) and username (_username)
 * - Creation and last activity timestamps for expiration tracking
 * - Logic to determine session validity based on timeout
 * 
 * Simple data container for session information.
 * Value-type class: copyable, assignable, holds its own data.
 */

class Session
{
	private:

	static const time_t					_SESSION_TIMEOUT;

	std::string							_sessionId;
	std::string							_username;
	time_t								_firstActivity;
	time_t								_lastActivity;

	public:

	Session();
	Session(const std::string& sessionId, const std::string& username);
	Session(Session const& other);
	Session& operator=(Session const& other);
	~Session();

	const std::string&	getSessionId() const;
	const std::string&	getUsername() const;
	time_t				getFirstActivity() const;
	time_t				getLastActivity() const;
	std::string			getSessionAge() const;
	static time_t		getTimeout();

	bool	isSessionIdEmpty() const;
	bool	isExpired() const;
	void	updateActivity();
};

std::ostream&	operator<<(std::ostream& os, const Session& session);

#endif
