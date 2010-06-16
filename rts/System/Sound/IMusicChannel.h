/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_MUSIC_CHANNEL_H
#define I_MUSIC_CHANNEL_H

#include <string>

#include "IAudioChannel.h"

class SoundSource;

class IMusicChannel : public IAudioChannel {
public:
	virtual void SetVolume(float volume) = 0;
	virtual void Enable(bool newState) = 0;

	/**
	 * @brief Start playing an ogg-file
	 * 
	 * NOT threadsafe, unlike the other functions!
	 * If another file is playing, it will stop it and play the new one instead.
	 */
	virtual void Play(const std::string& path, float volume = 1.0f, bool enqueue = false) = 0;

	virtual void Pause() = 0;

	/**
	 * @brief Stop playback
	 * 
	 * Don't call this if you just want to play another file (for performance).
	 */
	virtual void Stop() = 0;
	virtual float GetTime() = 0;
	virtual float GetPlayTime() = 0;
};

#ifdef    NO_SOUND
	class NullMusicChannel;
	#include "NullMusicChannel.h"
	typedef NullMusicChannel MusicChannelImpl;
#else  // NO_SOUND
	class MusicChannel;
	#include "MusicChannel.h"
	typedef MusicChannel MusicChannelImpl;
#endif // NO_SOUND

namespace Channels
{
	extern MusicChannelImpl BGMusic;
}

#endif // I_MUSIC_CHANNEL_H
