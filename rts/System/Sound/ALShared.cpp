#include "ALShared.h"

#include <al.h>

const CLogSubsystem LOG_SOUND("Sound", true);

bool CheckError(const char* msg)
{
	ALenum e = alGetError();
	if (e != AL_NO_ERROR)
	{
		LogObject(LOG_SOUND) << msg << ": " << (char*)alGetString(e);
		return false;
	}
	return true;
}