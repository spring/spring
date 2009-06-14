#ifndef MUSIC_H
#define MUSIC_H

#include <string>

#include "AudioChannel.h"

class SoundSource;

class MusicChannel : public AudioChannel
{
public:
	MusicChannel();
	~MusicChannel();

	void SetVolume(float volume);
	void Enable(bool newState);

	/**
	 * @brief Start playing an ogg-file
	 * 
	 * NOT threadsafe, unlike the other functions!
	 * If another file is playing, it will stop it and play the new one instead.
	 */
	void Play(const std::string& path, float volume = 1.0f);

	void Pause();

	/**
	 * @brief Stop playback
	 * 
	 * Don't call this if you just want to play another file (for performance).
	 */
	void Stop();
	unsigned int GetTime();
	unsigned int GetPlayTime();

private:
	SoundSource* current;
};

namespace Channels
{
	extern MusicChannel BGMusic;
};

#endif
