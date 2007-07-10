#ifndef _DEMO_H
#define _DEMO_H

#include <string>
#include <fstream>

#include "FileSystem/FileHandler.h"

/**
@brief base class for all demo stuff
Subclass other demo stuff from this
*/
class CDemo
{
public:
	
protected:
	std::string demoName;
};

/**
@brief Used to record demos
*/
class CDemoRecorder : public CDemo
{
public:
	CDemoRecorder();
	~CDemoRecorder();
	
	void SaveToDemo(const unsigned char* buf,const unsigned length);
	/**
	@brief assign a map name for the demo file
	When this function is called, we can rename our demo file so that map name / game time are visible. The demo file will be renamed by the destructor. Otherwise the name unnamed.sdf will be used.
	*/
	void SetName(const std::string& mapname);
	
private:
	std::ofstream* recordDemo;
	std::string wantedName;
};

class CDemoReader : public CDemo
{
public:
	CDemoReader(const std::string& filename);
	/**
	@brief read from demo file
	@return Amount of data read (bytes)
	*/
	unsigned GetData(unsigned char *buf, const unsigned length);
	
private:
	CFileHandler* playbackDemo;
	float demoTimeOffset;
	float nextDemoRead;
};


#endif
