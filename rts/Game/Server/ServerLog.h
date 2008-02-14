#ifndef SERVERLOG_H_
#define SERVERLOG_H_

#include <string>

/** @brief Interface for recieving server log messages
 * 
 * Subscibe with this class to ServerLogHandler
 * */
class ServerLog
{
public:	
	virtual void Message(const std::string& message) {};
	virtual void Warning(const std::string& message) {};
};

#endif /*SERVERLOG_H_*/
