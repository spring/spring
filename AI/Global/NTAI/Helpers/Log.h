#include "IAICallback.h"

class Log{
public:
	Log();
	virtual ~Log();
	Log& operator<< (int i);
	Log& operator<< (float f);
	Log& operator<< (const char* c);
	Log& operator<< (string s);

	string GameTime(); //returns Gametime in the format "[00:00]" min:sec
	void header(string message); // prints without any gametime, used mainly for headers, used for << operators aswell once I figure out howto code them
	void print(string message); // print to logfile
	void iprint(string message); //Print to info console and log
	void eprint(string message);
	void Open(bool plain=false); // Open Log file
	void Close(); // Close Logfile
	void Flush(); // Flushes the logfile
	void Message(string msg,int player); // recieved a message through the console
	bool Verbose(){
		if(verbose) verbose = false;
		else verbose = true;
		return verbose;
	}
	map<int,string> PlayerNames;
	bool FirstInstance();
	bool verbose;
	bool plaintext;
	ofstream logFile;
	Global* G;
	bool First;
};
