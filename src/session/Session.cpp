#include "session/Session.hpp"
#include "utils/PrintableString.hpp"
#include <sstream>

const time_t	Session::_SESSION_TIMEOUT = 600; // 10 minutes to demonstrate, can be changed for testing

Session::Session()
: _firstActivity(0)
, _lastActivity(0)
{}

Session::Session(const std::string& sessionId, const std::string& username)
: _sessionId(sessionId)
, _username(username)
, _firstActivity(std::time(0))
, _lastActivity(std::time(0))
{}

Session::Session(Session const& other)
: _sessionId(other._sessionId)
, _username(other._username)
, _firstActivity(other._firstActivity)
, _lastActivity(other._lastActivity)
, _data(other._data)
{}

Session& Session::operator=(Session const& other)
{
	if (this != &other)
	{
		_sessionId = other._sessionId;
		_username =other._username;
		_firstActivity= other._firstActivity;
		_lastActivity = other._lastActivity;
		_data = other._data;
	}
	return (*this);
}

Session::~Session()
{}

const std::string&	Session::getSessionId() const
{
	return _sessionId;
}

const std::string&	Session::getUsername() const
{
	return _username;
}

time_t	Session::getFirstActivity() const
{
	return _firstActivity;
}

time_t	Session::getLastActivity() const
{
	return _lastActivity;
}

std::string	Session::getData(const std::string& key) const
{
	std::map<std::string, std::string>::const_iterator it = _data.find(key);
	return (it != _data.end()) ? it->second : "";
}

size_t	Session::getDataCount() const
{
	return _data.size();
}

std::string	Session::getSessionAge() const
{
	time_t now = std::time(0);
	time_t age = now - _firstActivity;

	std::stringstream ss;
	if (age < 60)
		ss << age << "s";
	else if (age < 3600)
		ss << (age / 60) << "m" << (age % 60) << "s";
	else
		ss << (age / 3600) << "h" << ((age % 3600) / 60) << "m";
	return ss.str();
}

time_t	Session::getTimeout()
{
	return _SESSION_TIMEOUT;
}

void	Session::setData(const std::string& key, const std::string& value)
{
	_data[key] = value;
}

bool Session::isSessionIdEmpty() const
{
	return _sessionId.empty();
}

bool	Session::isExpired() const
{
	return (std::time(0) - _lastActivity) > _SESSION_TIMEOUT;
}

bool	Session::hasData(const std::string& key) const
{
	return _data.find(key) != _data.end();
}

void	Session::updateActivity()
{
	_lastActivity = std::time(0);
}

std::ostream&	operator<<(std::ostream& os, const Session& session)
{
	os << "Session Details" << "\n";
	os << "- SessionId: " << PrintableString(session.getSessionId()) << "\n";
	os << "- User: " << PrintableString(session.getUsername()) << "\n";
	os << "- Session Time : " << PrintableString(session.getSessionAge()) << "\n";
	os << "- Created: " << session.getFirstActivity() << "\n";
	os << "- Last Active: " << session.getLastActivity() << "\n";
	os << "- Session Expired: " << (session.isExpired() ? "Yes" : "No") << "\n";
	os << "- Data Count: " << session.getDataCount() << "\n";
	return os;
}
