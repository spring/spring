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
	DemoFileHeader fileHeader; //TODO should be const
protected:
	std::string demoName;
};


#endif
