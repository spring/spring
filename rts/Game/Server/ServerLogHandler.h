#ifndef SERVERLOGHANDLER_H_
#define SERVERLOGHANDLER_H_

#include <string>
#include <list>

#include <boost/format.hpp>

class ServerLog;

/** @brief Class for multicasting server messages
 * 
 * */
class ServerLogHandler
{
public:
	ServerLogHandler();
	~ServerLogHandler();
	
	void Subscribe(ServerLog* subscriber);
	void Unsubscribe(ServerLog* subscriber);
	
	void Message(const std::string& text);
	void Message(const boost::format& form);
	void Warning(const std::string& text);
	void Warning(const boost::format& form);
	
private:
	typedef std::list<ServerLog*> logList;
	logList subscribers;
};

#endif /*SERVERLOGHANDLER_H_*/
