#ifndef MUSIC_H
#define MUSIC_H

#include <string>

namespace music
{
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
	void SetVolume(float volume);
};

#endif
