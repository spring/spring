#include "ServerLogHandler.h"

#include "ServerLog.h"

ServerLogHandler::ServerLogHandler()
{
}

ServerLogHandler::~ServerLogHandler()
{
}

void ServerLogHandler::Subscribe(ServerLog* subscriber)
{
	subscribers.push_back(subscriber);
}

void ServerLogHandler::Unsubscribe(ServerLog* subscriber)
{
	for (logList::iterator it = subscribers.begin(); it != subscribers.end(); ++it)
	{
		if ((*it) == subscriber)
		{
			subscribers.erase(it);
			return;
		}
	}
	// silently ignore when not found
}

void ServerLogHandler::Message(const std::string& text)
{
	for (logList::iterator it = subscribers.begin(); it != subscribers.end(); ++it)
	{
		(*it)->Message(text);
	}
}

void ServerLogHandler::Message(const boost::format& form)
{
	Message(str(form));
}

void ServerLogHandler::Warning(const std::string& text)
{
	for (logList::iterator it = subscribers.begin(); it != subscribers.end(); ++it)
	{
		(*it)->Warning(text);
	}
}

void ServerLogHandler::Warning(const boost::format& form)
{
	Warning(str(form));
}

