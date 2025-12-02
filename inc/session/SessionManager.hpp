#ifndef SESSIONMANAGER_HPP
# define SESSIONMANAGER_HPP

# include "Session.hpp"
# include <string>
# include <map>
/**  
 * Centralized manager for all active user sessions.
 *
 * Responsibilities:
 * - Creates, validates, renews, and destroys sessions
 * - Generates unique session identifiers
 * - Maintains collection of all active Session objects
 * - Performs automatic cleanup of expired sessions
 *
 * This will ensure only one instance manages all sessions across the application.
 */

class SessionManager
{
	private:

	std::map<std::string, Session>	_sessions;
	time_t							_lastCleanup;

	// Not instantiable
	SessionManager();
	SessionManager(const SessionManager&);
	SessionManager& operator=(const SessionManager&);
	~SessionManager();

	void	_cleanupExpiredSessions();

	public:

	static const std::string	COOKIE_NAME;
	static const std::string	COOKIE_PATH;

	static		SessionManager& getInstance();
	std::string	createSession(const std::string& username);
	Session&	getSession(const std::string& sessionId);
	bool		isValidSession(const std::string& sessionId);
	void		destroySession(const std::string& sessionId);
	void		renewSession(const std::string& sessionId);
	std::string	generateSessionId();

	void		setSessionData(const std::string& sessionId, const std::string& key, const std::string& value);
	std::string getSessionData(const std::string& sessionId, const std::string& key);
};

#endif
