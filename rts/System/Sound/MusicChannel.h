/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MUSIC_CHANNEL_H
#define MUSIC_CHANNEL_H

#include <string>

#include "IMusicChannel.h"

class SoundSource;

class MusicChannel : public IMusicChannel {
public:
	MusicChannel();
	~MusicChannel();

	virtual void SetVolume(float volume);
	virtual void Enable(bool newState);

	/**
	 * @brief Start playing an ogg-file
	 * 
	 * NOT threadsafe, unlike the other functions!
	 * If another file is playing, it will stop it and play the new one instead.
	 */
	virtual void Play(const std::string& path, float volume = 1.0f, bool enqueue = false);

	virtual void Pause();

	/**
	 * @brief Stop playback
	 * 
	 * Do not call this if you just want to play another file (for performance).
	 */
	virtual void Stop();
	virtual float GetTime();
	virtual float GetPlayTime();

private:
	SoundSource* current;
};

#endif // MUSIC_CHANNEL_H
