#include "AudioChannel.h"

#include "Sound.h"
#include "Sim/Units/Unit.h"

namespace Channels
{
	EffectChannel General;
	EffectChannel Battle;
	EffectChannel UnitReply;
	EffectChannel UserInterface;
}

AudioChannel::AudioChannel() : volume(1.0f), enabled(true)
{
};

EffectChannel::EffectChannel() : emmitsPerFrame(32)
{
};

void EffectChannel::PlaySample(size_t id, float ovolume)
{
	if (enabled && emmitsThisFrame < emmitsPerFrame)
	{
		emmitsThisFrame++;
		sound->PlaySample(id, float3(0.0, 0.0, -1.0), ZeroVector, volume * ovolume, true);
	}
}

void EffectChannel::PlaySample(size_t id, const float3& p, float ovolume)
{
	if (enabled && emmitsThisFrame < emmitsPerFrame)
	{
		emmitsThisFrame++;
		sound->PlaySample(id, p, ZeroVector, volume * ovolume, false);
	}
}

void EffectChannel::PlaySample(size_t id, const float3& p, const float3& velocity, float ovolume)
{
	if (enabled && emmitsThisFrame < emmitsPerFrame)
	{
		emmitsThisFrame++;
		sound->PlaySample(id, p, velocity, volume * ovolume, false);
	}
}

void EffectChannel::PlaySample(size_t id, CUnit* u,float ovolume)
{
	if (enabled && emmitsThisFrame < emmitsPerFrame)
	{
		emmitsThisFrame++;
		sound->PlaySample(id, u->pos, u->speed, volume * ovolume, false);
	}
}

void EffectChannel::PlaySample(size_t id, CWorldObject* p,float ovolume)
{
	if (enabled && emmitsThisFrame < emmitsPerFrame)
	{
		emmitsThisFrame++;
		sound->PlaySample(id, p->pos, ZeroVector, volume * ovolume, false);
	}
} 
