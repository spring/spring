#ifndef MUSIC_H
#define MUSIC_H

#include <string>

namespace music
{
	void Play(const std::string& path, float volume = 1.0f);
	void Pause();
	void Stop();
	unsigned int GetTime();
	unsigned int GetPlayTime();
	void SetVolume(float volume);
};

#endif
