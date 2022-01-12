/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef NULL_AUDIO_CHANNEL_H
#define NULL_AUDIO_CHANNEL_H

#include "System/Sound/IAudioChannel.h"


class NullAudioChannel : public IAudioChannel {
public:
	~NullAudioChannel() {}

	void Enable(bool newState) override {}
	void SetVolume(float newVolume) override {}

	void PlaySample(size_t id, float volume = 1.0f) override {}
	void PlaySample(size_t id, const float3& p, float volume = 1.0f) override {}
	void PlaySample(size_t id, const float3& p, const float3& velocity, float volume = 1.0f) override {}

	void PlaySample(size_t id, const CWorldObject* p, float volume = 1.0f) override {}

	void PlayRandomSample(const GuiSoundSet& soundSet, const CWorldObject* obj) override {}
	void PlayRandomSample(const GuiSoundSet& soundSet, const float3& pos, const float3& vel = ZeroVector) override {}

	void StreamPlay(const std::string& path, float volume = 1.0f, bool enqueue = false) override {}

	void StreamStop() override {}
	void StreamPause() override {}
	float StreamGetTime() override { return 0.f; }
	float StreamGetPlayTime() override { return 0.f; }

protected:
	void FindSourceAndPlay(size_t id, const float3& p, const float3& velocity, float volume, bool relative) override {}
	void SoundSourceFinished(CSoundSource* sndSource) override {}

	friend class CSoundSource;
};

#endif // NULL_AUDIO_CHANNEL_H
