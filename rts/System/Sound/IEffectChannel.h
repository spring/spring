/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_EFFECT_CHANNEL_H
#define I_EFFECT_CHANNEL_H

#include "IAudioChannel.h"

#include <cstring> // for size_t

class float3;
class CWorldObject;
class CUnit;

class IEffectChannel : public IAudioChannel {
protected:
	IEffectChannel();

public:
	virtual void PlaySample(size_t id, float volume = 1.0f) = 0;
	virtual void PlaySample(size_t id, const float3& p, float volume = 1.0f) = 0;
	virtual void PlaySample(size_t id, const float3& p, const float3& velocity, float volume = 1.0f) = 0;

	virtual void PlaySample(size_t id, const CUnit* u, float volume = 1.0f) = 0;
	virtual void PlaySample(size_t id, const CWorldObject* p, float volume = 1.0f) = 0;

	void UpdateFrame() {
		emmitsThisFrame = 0;
	}
	void SetMaxEmmits(unsigned max) {
		emmitsPerFrame = max;
	}

protected:
	unsigned emmitsPerFrame;
	unsigned emmitsThisFrame;
};

#endif // I_EFFECT_CHANNEL_H
