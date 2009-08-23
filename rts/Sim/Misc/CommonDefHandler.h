#ifndef COMMONDEFHANDLER_H
#define COMMONDEFHANDLER_H

#include <string>

class CommonDefHandler
{
public:
	/// Loads a soundfile, does add sounds/ and wav extension if necessary
	int LoadSoundFile(const std::string& fileName);
};

#endif