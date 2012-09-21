/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COMMON_DEF_HANDLER_H
#define COMMON_DEF_HANDLER_H

#include <string>

class CommonDefHandler
{
public:
	/**
	 * Loads a soundfile, does add "sounds/" prefix
	 * and ".wav" extension if necessary.
	 */
	static int LoadSoundFile(const std::string& fileName);
};

#endif // COMMON_DEF_HANDLER
