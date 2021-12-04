#include "LogFile.h"

#include "RAI.h"
#include "LegacyCpp/IAICallback.h"

#include <string>
//#include <fstream>
//#include <stdlib.h>
//#include <iostream>
#include <stdio.h>

using std::string;

cLogFile::cLogFile(IAICallback* cb, string sFilename, bool bAppend)
{
	string sFilename_w;
	const bool located = cRAI::LocateFile(cb, sFilename, sFilename_w, true);
	if (!located) {
		logFile = stderr;
		fprintf(logFile, "RAI: Couldn't locate %s\n", sFilename.c_str());
		return;
	}

	if( bAppend )
		logFile = fopen(sFilename_w.c_str(), "a");
//		logFile = new ofstream();
//		logFile->open( logFileName.c_str(), ios::app );
	else
		logFile = fopen(sFilename_w.c_str(), "w");
//		ofstream oLog( logFileName.c_str() );
//		oLog.close();

/*
		pFile = fopen(logFileName.c_str(),"wt");
		fclose(pFile);
*/
}

cLogFile::~cLogFile()
{
	if (logFile == stderr)
		return;
	fclose(logFile);
//	logFile->close();
//	delete logFile;
}

cLogFile& cLogFile::operator<<(float message)
{
	if( message - int(message) > 0.0 )
	{
		if( message < 1.0 && message > 0.0 )
			fprintf(logFile, "%1.3f", message);
		else
			fprintf(logFile, "%1.2f", message);
	}
	else
		fprintf(logFile, "%1.0f", message);
	return *this;
}
/*
cLogFile& cLogFile::operator<<(int message)
{
	fprintf(logFile, "%i", message);
	return *this;
}
*/
cLogFile& cLogFile::operator<<(string message)
{
	fprintf(logFile, "%s", message.c_str());
	return *this;
}

/*
void cLogFile::Write(string message)
{
	fprintf(logFile, "%s", message.c_str());
//  fputs(message.c_str(),logFile);
//  *logFile<<message.c_str();
//	logFile->flush();
}

void cLogFile::Write(float message)
{
	if( message - int(message) > 0.0 )
		fprintf(logFile, "%1.2f", message);
	else
		fprintf(logFile, "%1.0f", message);
//	*logFile<<message;
//	logFile->flush();
}

void cLogFile::Write(int message)
{
//	*logFile<<message;
//	logFile->flush();
}
*/
