// _____________________________________________________
//
// RAI - Skirmish AI for TA Spring
// Author: Reth / Michael Vadovszki
// _____________________________________________________

#ifndef RAI_LOGFILE_H
#define RAI_LOGFILE_H

#include "ExternalAI/IAICallback.h"

using std::string;

class cLogFile
{
public:
	cLogFile(string sFilename, bool bAppend=true);
	~cLogFile();

	cLogFile& operator<<(float message);
//	cLogFile& operator<<(const char* message);
	cLogFile& operator<<(string message);

	void Write(string message);
	void Write(float message);
//	void Write(int message);
private:
	string logFileName;
//	ofstream *logFile;
	FILE *logFile;
};

#endif
