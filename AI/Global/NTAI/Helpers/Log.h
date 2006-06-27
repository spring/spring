#ifndef LOGGER_H
#define LOGGER_H
#include "AICallback.h"

class Log{
public:
	Log();
	virtual ~Log();
	Log& operator<< (int i);
	Log& operator<< (float f);
	Log& operator<< (const char* c);
	Log& operator<< (string s);

	string FrameTime(); // returns the Frame Number in the " <xyz Frames> " format
	string GameTime(); //returns Game time in the format "[00:00]" min:sec
	void header(string message); // prints without any game time, used mainly for headers, used for << operators as well
	void print(string message); // print to log file
	void iprint(string message); //Print to info console and log
	void eprint(string message); //Print an error
	void Open(bool plain=false); // Open Log file
	void Close(); // Close Log file
	void Flush(); // Flushes the log file
	void Message(string msg,int player); // received a message through the console
	void Set(Global* GL);
	bool Verbose(){
		if(verbose == true){
			verbose = false;
		}else{
			verbose = true;
		}
		return verbose;
	}
	map<int,string> PlayerNames; // player id <> Player name
	bool FirstInstance(); // Is this the first instance of the AI/logger class
private:
	bool plaintext;
	bool verbose;
	ofstream logFile;
	bool First;
	Global* G;
};
#endif
