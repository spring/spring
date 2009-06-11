#ifndef MUSIC_H
#define MUSIC_H

#include <string>
#include <boost/thread/mutex.hpp>

#include "AudioChannel.h"
#include "OggStream.h"

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

	void Update();
	
private:
	COggStream oggStream;

	struct TrackItem
	{
		std::string path;
		float volume;
	};
	TrackItem nextTrack;
	volatile bool playNext;
	volatile bool playing;

	boost::mutex musicMutex;
};

namespace Channels
{
	extern MusicChannel BGMusic;
};

#endif
