/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_AUDIO_CHANNEL_H
#define I_AUDIO_CHANNEL_H

#include <string.h>
#include "System/float3.h"

class CSoundSource;
class CUnit;
class CWorldObject;

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
	virtual void Enable(bool newState) = 0;
	bool IsEnabled() const
	{
		return enabled;
	}

	/**
	 * @param newVolume [0.0, 1.0]
	 */
	virtual void SetVolume(float newVolume) = 0;
	/**
	 * @return [0.0, 1.0]
	 */
	float GetVolume() const
	{
		return volume;
	}

	virtual void PlaySample(size_t id, float volume = 1.0f) = 0;
	virtual void PlaySample(size_t id, const float3& p, float volume = 1.0f) = 0;
	virtual void PlaySample(size_t id, const float3& p, const float3& velocity, float volume = 1.0f) = 0;

	virtual void PlaySample(size_t id, const CUnit* u, float volume = 1.0f) = 0;
	virtual void PlaySample(size_t id, const CWorldObject* p, float volume = 1.0f) = 0;

	/**
	 * @brief Start playing an ogg-file
	 * 
	 * NOT threadsafe, unlike the other functions!
	 * If another file is playing, it will stop it and play the new one instead.
	 */
	virtual void StreamPlay(const std::string& path, float volume = 1.0f, bool enqueue = false) = 0;

	/**
	 * @brief Stop playback
	 * 
	 * Don't call this if you just want to play another file (for performance).
	 */
	virtual void StreamStop() = 0;
	virtual void StreamPause() = 0;
	virtual float StreamGetTime() = 0;
	virtual float StreamGetPlayTime() = 0;

	void UpdateFrame() {
		emmitsThisFrame = 0;
	}
	void SetMaxEmmits(unsigned max) {
		emmitsPerFrame = max;
	}

protected:
	virtual void FindSourceAndPlay(size_t id, const float3& p, const float3& velocity, float volume, bool relative) = 0;

	virtual void SoundSourceFinished(CSoundSource* sndSource) = 0;
	friend class CSoundSource;

public:
	float volume;
	bool enabled;

protected:
	unsigned emmitsPerFrame;
	unsigned emmitsThisFrame;
};

#endif // I_AUDIO_CHANNEL_H
