#ifndef _DEMO_H
#define _DEMO_H

#include <string>

#include "demofile.h"

/**
@brief base class for all demo stuff
Subclass other demo stuff from this
*/
class CDemo
{
public:
	
protected:
	std::string demoName;
	DemoFileHeader fileHeader;
};


#endif
