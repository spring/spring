/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef EFFECT_CHANNEL_H
#define EFFECT_CHANNEL_H

#include "IEffectChannel.h"

class EffectChannel : public IEffectChannel {
public:
	EffectChannel();

	virtual void PlaySample(size_t id, float volume = 1.0f);
	virtual void PlaySample(size_t id, const float3& p, float volume = 1.0f);
	virtual void PlaySample(size_t id, const float3& p, const float3& velocity, float volume = 1.0f);

	virtual void PlaySample(size_t id, const CUnit* u, float volume = 1.0f);
	virtual void PlaySample(size_t id, const CWorldObject* p, float volume = 1.0f);
};

#endif // EFFECT_CHANNEL_H
