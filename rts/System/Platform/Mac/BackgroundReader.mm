/*
 *  BackgroundReader.mm
 *  SpringRTS
 *  Copyright 2006 Lorenz Pretterhofer <krysole@internode.on.net>
 *
 *  NOTE: atm the Background reader is not being used afaik, so i havn't finished
 *  this implementation of it...if it becomes in use this will use the NSFileHandle
 *  cocoa api to load the file async (which is why its objective-c++ and not plain c++).
 */

#include "BackgroundReader.h"

/*
 * I use a second hidden class so that files that include BackgroundReader don't
 * have to be compiled objective-C++ (not automatic since they have a cpp and not
 * a mm extension).
 */
class MacBackgroundReader
{
public:
	MacBackgroundReader();
	virtual ~MacBackgroundReader();
private:
};

CBackgroundReader::CBackgroundReader()
{
	reader = new MacBackgroundReader();
}

CBackgroundReader::~CBackgroundReader()
{
	delete reader;
}

void CBackgroundReader::ReadFile(const char *filename, unsigned char *buffer, 
	int length, int priority, int *reportReader)
{
}

void CBackgroundReader::Update()
{
}

MacBackgroundReader::MacBackgroundReader()
{
}

MacBackgroundReader::~MacBackgroundReader()
{
}
