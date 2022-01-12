/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef AUDIO_CHANNEL_H
#define AUDIO_CHANNEL_H

#include <deque>
#include <cstring>

#include "System/Sound/IAudioChannel.h"
#include "System/UnorderedSet.hpp"

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
private:
	typedef std::pair<std::string, float> StreamQueueItem;

public:
	void Enable(bool newState) override;
	void SetVolume(float newVolume) override;

	void PlaySample(size_t id, float volume = 1.0f) override;
	void PlaySample(size_t id, const float3& pos, float volume = 1.0f) override;
	void PlaySample(size_t id, const float3& pos, const float3& velocity, float volume = 1.0f) override;

	void PlaySample(size_t id, const CWorldObject* obj, float volume = 1.0f) override;

	void PlayRandomSample(const GuiSoundSet& soundSet, const CWorldObject* obj) override;
	void PlayRandomSample(const GuiSoundSet& soundSet, const float3& pos, const float3& vel = ZeroVector) override;

	void StreamPlay(const StreamQueueItem& item, bool enqueue)  { StreamPlay(item.first, item.second, enqueue); }
	void StreamPlay(const std::string& path, float volume = 1.0f, bool enqueue = false) override;

	/**
	 * @brief Stop playback
	 *
	 * Do not call this if you just want to play another file (for performance).
	 */
	void StreamStop() override;
	void StreamPause() override;
	float StreamGetTime() override;
	float StreamGetPlayTime() override;

protected:
	void FindSourceAndPlay(size_t id, const float3& pos, const float3& velocity, float volume, bool relative) override;
	void SoundSourceFinished(CSoundSource* sndSource) override;

private:
	spring::unsynced_set<CSoundSource*> curSources;
	std::deque<StreamQueueItem> streamQueue;

	CSoundSource* curStreamSrc = nullptr;

	static constexpr size_t MAX_STREAM_QUEUESIZE = 10;
};

#endif // AUDIO_CHANNEL_H
