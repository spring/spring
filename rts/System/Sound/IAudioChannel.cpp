/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IAudioChannel.h"

IAudioChannel::IAudioChannel()
	: volume(1.0f)
	, enabled(true)
	, emitsPerFrame(1000)
	, emitsThisFrame(0)
	, maxConcurrentSources(1024)
{
}

