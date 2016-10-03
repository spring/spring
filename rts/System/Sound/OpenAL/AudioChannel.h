/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef AUDIO_CHANNEL_H
#define AUDIO_CHANNEL_H

#include <map>
#include <vector>
#include <string.h>

#include "System/Sound/IAudioChannel.h"

struct GuiSoundSet;
class CSoundSource;
class CWorldObject;

/**
 * @brief Channel for playing sounds
 *
 * Has its own volume "slider", and can be enabled / disabled seperately.
 * Abstract base class.
 */
class AudioChannel : public IAudioChannel {
public:
	AudioChannel();
	~AudioChannel() {}

	void Enable(bool newState);
	void SetVolume(float newVolume);

	void PlaySample(size_t id, float volume = 1.0f);
	void PlaySample(size_t id, const float3& pos, float volume = 1.0f);
	void PlaySample(size_t id, const float3& pos, const float3& velocity, float volume = 1.0f);

	void PlaySample(size_t id, const CWorldObject* obj, float volume = 1.0f);

	void PlayRandomSample(const GuiSoundSet& soundSet, const CWorldObject* obj);
	void PlayRandomSample(const GuiSoundSet& soundSet, const float3& pos);

	void StreamPlay(const std::string& path, float volume = 1.0f, bool enqueue = false);

	/**
	 * @brief Stop playback
	 *
	 * Do not call this if you just want to play another file (for performance).
	 */
	void StreamStop();
	void StreamPause();
	float StreamGetTime();
	float StreamGetPlayTime();

protected:
	void FindSourceAndPlay(size_t id, const float3& pos, const float3& velocity, float volume, bool relative);

	void SoundSourceFinished(CSoundSource* sndSource);

private:
	std::map<CSoundSource*, bool> cur_sources;

	//! streams
	struct StreamQueueItem {
		StreamQueueItem() : volume(0.f) {}
		StreamQueueItem(const std::string& fileName, float& volume)
			: fileName(fileName)
			, volume(volume)
		{}
		std::string fileName;
		float volume;
	};

	CSoundSource* curStreamSrc;
	std::vector<StreamQueueItem> streamQueue;
	static const size_t MAX_STREAM_QUEUESIZE;
};

#endif // AUDIO_CHANNEL_H
