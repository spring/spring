/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ALShared.h"

#include "SoundLog.h"


bool CheckError(const char* msg)
{
	ALenum e = alGetError();
	if (e != AL_NO_ERROR)
	{
		char *alerr = (char*)alGetString(e);
		std::string err = msg;
		err += ": ";
		err += (alerr != NULL) ? alerr : "Unknown error";

		LogObject(LOG_SOUND) << err.c_str();
		return false;
	}
	return true;
}
