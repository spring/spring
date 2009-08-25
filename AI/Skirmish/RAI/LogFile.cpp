#include "LogFile.h"

#include "ExternalAI/IAICallback.h"

#include <string>
//#include <fstream>
//#include <stdlib.h>
//#include <iostream>
#include <stdio.h>

using std::string;

cLogFile::cLogFile(IAICallback* cb, string sFilename, bool bAppend)
{
	logFileName = cLogFile::GetDir(cb) + sFilename;
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

std::string cLogFile::GetDir(IAICallback* cb, bool writeableAndCreate, std::string relPath) {

	std::string returnedPath = "";

	AIHCGetDataDir cmdData = {
		relPath.c_str(),
		writeableAndCreate,
		writeableAndCreate,
		true,
		false,
		NULL
	};

	int ret = cb->HandleCommand(AIHCGetDataDirId, &cmdData);

	if (ret == 1 && cmdData.ret_path != NULL) {
		// nothing failed
		std::string returnedPath = std::string(cmdData.ret_path);
		free(cmdData.ret_path);
		cmdData.ret_path = NULL;
	}

	return returnedPath;
}
