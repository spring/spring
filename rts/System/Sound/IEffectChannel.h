/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_EFFECT_CHANNEL_H
#define I_EFFECT_CHANNEL_H

#include "IAudioChannel.h"

#include "float3.h"
#include <cstring> // for size_t

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

#ifdef    NO_SOUND
	class NullEffectChannel;
	#include "NullEffectChannel.h"
	typedef NullEffectChannel EffectChannelImpl;
#else  // NO_SOUND
	class EffectChannel;
	#include "EffectChannel.h"
	typedef EffectChannel EffectChannelImpl;
#endif // NO_SOUND

/**
 * @brief If you want to play a sound, use one of these
 */
namespace Channels
{
	extern EffectChannelImpl General;
	extern EffectChannelImpl Battle;
	extern EffectChannelImpl UnitReply;
	extern EffectChannelImpl UserInterface;
}

#endif // I_EFFECT_CHANNEL_H
