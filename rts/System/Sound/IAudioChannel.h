/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_AUDIO_CHANNEL_H
#define I_AUDIO_CHANNEL_H

#include <map>
#include <vector>
#include <string.h>
#include "float3.h"

class CSoundSource;

/**
 * @brief Channel for playing sounds
 *
 * Has its own volume "slider", and can be enabled / disabled seperately.
 * Abstract base class.
 */
class IAudioChannel {
protected:
	IAudioChannel();

public:
	virtual void Enable(bool newState);
	bool IsEnabled() const
	{
		return enabled;
	}

	virtual void SetVolume(float newVolume);
	float GetVolume() const
	{
		return volume;
	}

	/**
	 * @brief Start playing an ogg-file
	 * 
	 * NOT threadsafe, unlike the other functions!
	 * If another file is playing, it will stop it and play the new one instead.
	 */
	virtual void StreamPlay(const std::string& path, float volume = 1.0f, bool enqueue = false);

	/**
	 * @brief Stop playback
	 * 
	 * Don't call this if you just want to play another file (for performance).
	 */
	virtual void StreamStop();
	virtual void StreamPause();
	virtual float StreamGetTime();
	virtual float StreamGetPlayTime();

protected:
	virtual void FindSourceAndPlay(size_t id, const float3& p, const float3& velocity, float volume, bool relative);

	virtual void SoundSourceFinished(CSoundSource* sndSource);
	friend class CSoundSource;

public:
	float volume;
	bool enabled;
	std::map<CSoundSource*, bool> cur_sources;

	//! streams
	struct StreamQueueItem {
		StreamQueueItem() : volume(0.f) {}
		StreamQueueItem(const std::string& f, float& v) : filename(f), volume(v) {}
		std::string filename;
		float volume;
	};
	
	CSoundSource* curStreamSrc;
	std::vector<StreamQueueItem> streamQueue;
	static const size_t MAX_STREAM_QUEUESIZE;
};

#endif // I_AUDIO_CHANNEL_H
