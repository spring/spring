#ifndef ALShared
#define ALShared

#include <AL/al.h>

#include "LogOutput.h"

static const CLogSubsystem LOG_SOUND("Sound", true);

static bool CheckError(const char* msg)
{
	ALenum e = alGetError();
	if (e != AL_NO_ERROR)
	{
		LogObject(LOG_SOUND) << msg << ": " << (char*)alGetString(e);
		return false;
	}
	return true;
}

#endif
