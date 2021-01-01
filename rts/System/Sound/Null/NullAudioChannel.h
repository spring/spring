/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef NULL_AUDIO_CHANNEL_H
#define NULL_AUDIO_CHANNEL_H

#include "System/Sound/IAudioChannel.h"


class NullAudioChannel : public IAudioChannel {
public:
	~NullAudioChannel() {}

	void Enable(bool newState) {}
	void SetVolume(float newVolume) {}

	void PlaySample(size_t id, float volume = 1.0f) {}
	void PlaySample(size_t id, const float3& p, float volume = 1.0f) {}
	void PlaySample(size_t id, const float3& p, const float3& velocity, float volume = 1.0f) {}

	void PlaySample(size_t id, const CWorldObject* p, float volume = 1.0f) {}

	void PlayRandomSample(const GuiSoundSet& soundSet, const CWorldObject* obj) {}
	void PlayRandomSample(const GuiSoundSet& soundSet, const float3& pos, const float3& vel = ZeroVector) {}

	void StreamPlay(const std::string& path, float volume = 1.0f, bool enqueue = false) {}

	void StreamStop() {}
	void StreamPause() {}
	float StreamGetTime() { return 0.f; }
	float StreamGetPlayTime() { return 0.f; }

protected:
	void FindSourceAndPlay(size_t id, const float3& p, const float3& velocity, float volume, bool relative) override {}
	void SoundSourceFinished(CSoundSource* sndSource) override {}

	friend class CSoundSource;
};

#endif // NULL_AUDIO_CHANNEL_H
