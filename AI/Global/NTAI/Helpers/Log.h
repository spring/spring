#include "IAICallback.h"

class Global;

class Log{
public:
	Log(Global* GL);
	virtual ~Log();
	string GameTime(); //returns Gametime in the format "[00:00]" min:sec
	void print(string message); // print to logfile
	void iprint(string message); //Print to info console and log
	void eprint(string message);
	void Log::Open(bool plain=false); // Open Log file
	void Close(); // Close Logfile
	void Flush(); // Flushes the logfile
	void Message(string msg,int player); // recieved a message through the console
	bool Verbose(){
		if(verbose) verbose = false;
		else verbose = true;
		return verbose;
	}
	bool verbose;
	bool plaintext;
	Global* G;
};
