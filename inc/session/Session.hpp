#ifndef SESSION_HPP
# define SESSION_HPP

# include <string>
# include <ctime>
# include <map>
/**
 * Represents a single user session with identity, timing, and custom data
 * 
 * Each Session object contains:
 * - Private attributes: session ID (_sessionId) and username (_username)
 * - Creation and last activity timestamps for expiration tracking
 * - Flexible key-value storage for session-specific data
 * - Logic to determine session validity based on timeout
 * 
 * This class is a data container for individual user sessions!
 */

class Session
{
	private:

	static const time_t					_SESSION_TIMEOUT;

	std::string							_sessionId;
	std::string							_username;
	time_t								_firstActivity;
	time_t								_lastActivity;
	std::map<std::string, std::string>	_data;

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
	std::string			getData(const std::string& key) const;
	size_t				getDataCount() const;
	std::string			getSessionAge() const;

	void	setData(const std::string& key, const std::string& value);

	bool	isSessionIdEmpty() const;
	bool	isExpired() const;
	bool	hasData(const std::string& key) const;
	void	updateActivity();
};

std::ostream&	operator<<(std::ostream& os, const Session& session);

#endif
