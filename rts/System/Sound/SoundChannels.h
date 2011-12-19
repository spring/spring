/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SOUND_CHANNELS_H
#define SOUND_CHANNELS_H

#ifdef NO_SOUND
	#include "NullAudioChannel.h"

	typedef NullAudioChannel AudioChannelImpl;
#else
	#include "AudioChannel.h"

	typedef AudioChannel AudioChannelImpl;
#endif

/**
* @brief If you want to play a sound, use one of these
*/
namespace Channels {
	extern AudioChannelImpl BGMusic;
	extern AudioChannelImpl General;
	extern AudioChannelImpl Battle;
	extern AudioChannelImpl UnitReply;
	extern AudioChannelImpl UserInterface;
};

#endif
