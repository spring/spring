/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SOUND_CHANNELS_H
#define SOUND_CHANNELS_H

#ifdef NO_SOUND
	#include "NullEffectChannel.h"
	#include "NullMusicChannel.h"

	typedef NullEffectChannel EffectChannelImpl;
	typedef NullMusicChannel MusicChannelImpl;
#else
	#include "EffectChannel.h"
	#include "MusicChannel.h"

	typedef EffectChannel EffectChannelImpl;
	typedef MusicChannel MusicChannelImpl;
#endif

/**
* @brief If you want to play a sound, use one of these
*/
namespace Channels {
	extern MusicChannelImpl BGMusic;
	extern EffectChannelImpl General;
	extern EffectChannelImpl Battle;
	extern EffectChannelImpl UnitReply;
	extern EffectChannelImpl UserInterface;
};

#endif
