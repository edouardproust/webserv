#include "session/SessionManager.hpp"
#include "utils/utils.hpp"
#include "utils/Log.hpp"
#include <sstream>

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

std::string	SessionManager::createSession(const std::string& username)
{
	std::string sessionId = generateSessionId();
	_sessions[sessionId] = Session(sessionId, username);
	return sessionId;
}

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

void	SessionManager::setSessionData(const std::string& sessionId, const std::string& key, const std::string& value)
{
	Session& session = getSession(sessionId);
	if (!session.getSessionId().empty())
		session.setData(key, value);
}

bool SessionManager::isValidSession(const std::string& sessionId)
{
    _cleanupExpiredSessions();
    
    // Debug logging
    Log::dev("debug", "=== Session Validation ===");
    Log::dev("debug", "Looking for session ID: " + sessionId);
    Log::dev("debug", "Total sessions in map: " + utils::str(_sessions.size()));
    
    // Log all current sessions for debugging
    for (std::map<std::string, Session>::iterator it = _sessions.begin(); it != _sessions.end(); ++it) {
        Log::dev("debug", "Session in map: " + it->first + " -> " + it->second.getUsername());
    }
    
    std::map<std::string, Session>::iterator it = _sessions.find(sessionId);
    bool exists = (it != _sessions.end());
    bool expired = exists ? it->second.isExpired() : true;
    
    Log::dev("debug", "Session exists: " + std::string(exists ? "YES" : "NO"));
    if (exists) {
        Log::dev("debug", "Session expired: " + std::string(expired ? "YES" : "NO"));
        Log::dev("debug", "Session username: " + it->second.getUsername());
    }
    Log::dev("debug", "Validation result: " + std::string((exists && !expired) ? "VALID" : "INVALID"));
    
    return (exists && !expired);
}

void	SessionManager::destroySession(const std::string& sessionId)
{
	_sessions.erase(sessionId);
}

void	SessionManager::renewSession(const std::string& sessionId)
{
	Session& session = getSession(sessionId);
	if (!session.getSessionId().empty())
		session.updateActivity();
}

std::string	SessionManager::getSessionData(const std::string& sessionId, const std::string& key)
{
	Session& session = getSession(sessionId);
	return session.getData(key);
}

std::string	SessionManager::generateSessionId()
{
	static int counter = 0;
	std::stringstream ss;
	ss << "ssid_" << ++counter << "_" << std::time(0);
	return ss.str();
}

void	SessionManager::_cleanupExpiredSessions()
{
	time_t now = std::time(0);
	if (now - _lastCleanup < 300)
		return;
	_lastCleanup = now;
	for (std::map<std::string, Session>::iterator it = _sessions.begin(); it != _sessions.end();) {
		if (it->second.isExpired())
			_sessions.erase(it++);
		else
			++it;
	}
}
