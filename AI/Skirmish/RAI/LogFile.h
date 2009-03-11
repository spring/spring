// _____________________________________________________
//
// RAI - Skirmish AI for TA Spring
// Author: Reth / Michael Vadovszki
// _____________________________________________________

#ifndef RAI_LOGFILE_H
#define RAI_LOGFILE_H

#include "ExternalAI/IAICallback.h"
#include "AIExport.h"
using std::string;

class cLogFile
{
public:
	cLogFile(string sFilename, bool bAppend=true);
	~cLogFile();

	cLogFile& operator<<(float message);
	cLogFile& operator<<(string message);

	static std::string GetDir(bool writeableAndCreate = true, std::string relPath = "");

private:
	string logFileName;
	FILE *logFile;
};

#endif
