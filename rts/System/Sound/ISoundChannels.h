/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SOUND_CHANNELS_H
#define SOUND_CHANNELS_H

#include "IAudioChannel.h"
/**
* @brief If you want to play a sound, use one of these
*/
namespace Channels {
	extern IAudioChannel* BGMusic;
	extern IAudioChannel* General;
	extern IAudioChannel* Battle;
	extern IAudioChannel* UnitReply;
	extern IAudioChannel* UserInterface;
}

#endif
