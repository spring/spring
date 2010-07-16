/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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
	CDemo();
	virtual ~CDemo() {};

	const DemoFileHeader& GetFileHeader() const { return fileHeader; }

protected:
	DemoFileHeader fileHeader;
	std::string demoName;
};

#endif // _DEMO_H

