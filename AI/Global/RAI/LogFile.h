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
	cLogFile& operator<<(string message);

	static const std::string& GetRAIRootDirectory();

private:
	string logFileName;
	FILE *logFile;
};

#endif
