// _____________________________________________________
//
// RAI - Skirmish AI for Spring
// Author: Reth / Michael Vadovszki
// _____________________________________________________

#ifndef RAI_LOGFILE_H
#define RAI_LOGFILE_H

#include <string>
#include <cstdio>

using std::string;
namespace springLegacyAI {
	class IAICallback;
}
using namespace springLegacyAI;


class cLogFile
{
public:
	cLogFile(IAICallback* cb, string sFilename, bool bAppend=true);
	~cLogFile();

	cLogFile& operator<<(float message);
	cLogFile& operator<<(string message);

private:
	string logFileName;
	FILE *logFile;
};

#endif // RAI_LOGFILE_H
