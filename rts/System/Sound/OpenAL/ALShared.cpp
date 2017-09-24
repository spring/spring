/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>
#include <cstddef>

#include "ALShared.h"

#include "System/Sound/SoundLog.h"
#include <alc.h>

bool CheckError(const char* msg)
{
	// alGetError() always returns an error without context -> don't call it
	assert(alcGetCurrentContext() != nullptr);

	const ALenum e = alGetError();
	const char* err;
	switch (e) {
		case AL_INVALID_NAME:
			err = "AL_INVALID_NAME";
			break;
		case AL_INVALID_ENUM:
			err = "AL_INVALID_ENUM";
			break;
		case AL_INVALID_VALUE:
			err = "AL_INVALID_VALUE";
			break;
		case AL_INVALID_OPERATION:
			err = "AL_INVALID_OPERATION";
			break;
		case AL_OUT_OF_MEMORY:
			err = "AL_OUT_OF_MEMORY";
			break;
		default:
			err = "Unknown error";
			break;
		case AL_NO_ERROR:
			return true;
	}
	LOG_L(L_ERROR, "%s: %s (%d)", msg, err, e);
	return false;
}
