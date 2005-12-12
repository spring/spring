#ifndef LOG_H
#define LOG_H

#include "IAICallback.h"

class Log{
public:
	Log(IAICallback* aicb);
	~Log();
	string GameTime(); //returns Gametime in the format "[00:00]" min:sec
	void print(string message); // print to logfile
	void iprint(string message); //Print to info console and log
	void Log::Open(); // Open Log file
	void Close(); // Close Logfile
	void Flush(); // Flushes the logfile
	void Message(string msg,int player); // recieved a message through the console
};

#endif
