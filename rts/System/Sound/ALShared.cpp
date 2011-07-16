/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ALShared.h"

#include "SoundLog.h"

#include <stddef.h>


bool CheckError(const char* msg)
{
	ALenum e = alGetError();
	const bool hasError = (e != AL_NO_ERROR);
	if (hasError) {
		char* alerr = (char*)alGetString(e);
		LOG_L(L_ERROR, "%s: %s",
				msg, ((alerr != NULL) ? alerr : "Unknown error"));
	}

	return !hasError;
}
