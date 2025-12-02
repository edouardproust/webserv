#include "session/SessionManager.hpp"
#include "utils/utils.hpp"
#include "utils/Log.hpp"
#include <sstream>

const std::string	SessionManager::COOKIE_NAME = "session_id";
const std::string	SessionManager::COOKIE_PATH = "/";

SessionManager::SessionManager()
: _lastCleanup(std::time(0))
{}

SessionManager::~SessionManager()
{}

SessionManager& SessionManager::getInstance()
{
	static SessionManager instance;
	return instance;
}

/**
 * Creates a new session with auto-generated ID and stores it.
 * Session starts with current timestamp as both first and last activity.
 */
std::string	SessionManager::createSession(const std::string& username)
{
	std::string sessionId = generateSessionId();
	_sessions[sessionId] = Session(sessionId, username);
	Log::dev("debug", "Session created:\n" + utils::str(_sessions[sessionId]));
	return sessionId;
}

/**
 * Retrieves session if it exists and is not expired.
 * Updates session's last activity timestamp on access.
 * Returns empty session object if session not found or expired.
 */
Session&	SessionManager::getSession(const std::string& sessionId)
{
	_cleanupExpiredSessions();
	std::map<std::string, Session>::iterator it = _sessions.find(sessionId);
	if (it != _sessions.end() && !it->second.isExpired()) {
		it->second.updateActivity();
		return it->second;
	}
	static Session emptySession;
	return emptySession;
}

/**
 * Stores custom data in session's key-value store.
 * Only stores data if session exists and is valid.
 */
void	SessionManager::setSessionData(const std::string& sessionId, const std::string& key, const std::string& value)
{
	Session& session = getSession(sessionId);
	if (!session.getSessionId().empty())
		session.setData(key, value);
}

/**
 * Validates session existence and expiration status.
 * Does not update activity timestamp (unlike getSession()).
 */
bool SessionManager::isValidSession(const std::string& sessionId)
{
	_cleanupExpiredSessions();
	std::map<std::string, Session>::iterator it = _sessions.find(sessionId);
	return (it != _sessions.end() && !it->second.isExpired());
}

/**
 * Checks and removes session from active sessions map.
 * Used for explicit logout operations.
 */
void	SessionManager::destroySession(const std::string& sessionId)
{
	std::map<std::string, Session>::iterator it = _sessions.find(sessionId);
	if (it != _sessions.end()) {
		Log::dev("debug", "Destroying session: " + sessionId);
		_sessions.erase(sessionId);
	}
}

/**
 * Updates session's last activity timestamp to current time.
 * Effectively resets the session timeout counter.
 */
void	SessionManager::renewSession(const std::string& sessionId)
{
	Session& session = getSession(sessionId);
	if (!session.getSessionId().empty())
		session.updateActivity();
}

/**
 * Retrieves data from session's key-value store.
 * Returns empty string if session not found or key doesn't exist.
 */
std::string	SessionManager::getSessionData(const std::string& sessionId, const std::string& key)
{
	Session& session = getSession(sessionId);
	return session.getData(key);
}

/**
 * Generates unique session ID with format: "ssid_<counter>_<timestamp>"
 * Counter increments on each call, timestamp is Unix epoch seconds.
 */
std::string	SessionManager::generateSessionId()
{
	static int counter = 0;
	std::stringstream ss;
	ss << "ssid_" << ++counter << "_" << std::time(0);
	return ss.str();
}

/**
 * Cleans up expired sessions (older than SESSION_TIMEOUT).
 */
void	SessionManager::_cleanupExpiredSessions()
{
	for (std::map<std::string, Session>::iterator it = _sessions.begin(); it != _sessions.end();) {
		if (it->second.isExpired())
			_sessions.erase(it++);
		else
			++it;
	}
}
