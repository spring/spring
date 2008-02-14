#ifndef EVENTPRINTER_H_
#define EVENTPRINTER_H_

#include "Game/Server/ServerLog.h"

class EventPrinter : ServerLog
{
public:
	virtual ~EventPrinter();
	
	virtual void Message(const std::string& message);
	virtual void Warning(const std::string& message);
};


#endif /*EVENTPRINTER_H_*/
