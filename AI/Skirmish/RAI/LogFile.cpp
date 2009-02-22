#include "LogFile.h"

//#include <fstream>
//#include <stdlib.h>
//#include <iostream>
#include <stdio.h>

using namespace std;

cLogFile::cLogFile(string sFilename, bool bAppend)
{
	logFileName = cLogFile::GetRAIRootDirectory() + sFilename;
	if( bAppend )
		logFile = fopen(logFileName.c_str(),"a");
//		logFile = new ofstream();
//		logFile->open( logFileName.c_str(), ios::app );
	else
		logFile = fopen(logFileName.c_str(),"w");
//		ofstream oLog( logFileName.c_str() );
//		oLog.close();

/*
		pFile = fopen(logFileName.c_str(),"wt");
		fclose(pFile);
*/
}

cLogFile::~cLogFile()
{
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

const std::string& cLogFile::GetRAIRootDirectory() {

	static std::string raiRootDirectory = aiexport_getDataDir(false);
	return raiRootDirectory;
}
