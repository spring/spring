// _____________________________________________________
//
// RAI - Skirmish AI for TA Spring
// Author: Reth / Michael Vadovszki
// _____________________________________________________

#ifndef RAI_LOGFILE_H
#define RAI_LOGFILE_H

#include "ExternalAI/IAICallback.h"
using std::string;

const string RAIDirectory = "AI/RAI/";

class cLogFile
{
public:
	cLogFile(string sFilename, bool bAppend=true);
	~cLogFile();

	cLogFile& operator<<(float message);
	cLogFile& operator<<(string message);

private:
	string logFileName;
	FILE *logFile;
};

#endif
