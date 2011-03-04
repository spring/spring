/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "EffectChannel.h"

#include "ISound.h"
#include "Sim/Objects/WorldObject.h"
#include "Sim/Units/Unit.h"

EffectChannel::EffectChannel()
{
}

void EffectChannel::PlaySample(size_t id, float volume)
{
	if (emmitsThisFrame < emmitsPerFrame)
	{
		emmitsThisFrame++;
		FindSourceAndPlay(id, float3(0.0, 0.0, -1.0), ZeroVector, volume, true);
	}
}

void EffectChannel::PlaySample(size_t id, const float3& p, float volume)
{
	if (emmitsThisFrame < emmitsPerFrame)
	{
		emmitsThisFrame++;
		FindSourceAndPlay(id, p, ZeroVector, volume, false);
	}
}

void EffectChannel::PlaySample(size_t id, const float3& p, const float3& velocity, float volume)
{
	if (emmitsThisFrame < emmitsPerFrame)
	{
		emmitsThisFrame++;
		FindSourceAndPlay(id, p, velocity, volume, false);
	}
}

void EffectChannel::PlaySample(size_t id, const CUnit* u, float volume)
{
	if (emmitsThisFrame < emmitsPerFrame)
	{
		emmitsThisFrame++;
		FindSourceAndPlay(id, u->pos, u->speed, volume, false);
	}
}

void EffectChannel::PlaySample(size_t id, const CWorldObject* p, float volume)
{
	if (emmitsThisFrame < emmitsPerFrame)
	{
		emmitsThisFrame++;
		FindSourceAndPlay(id, p->pos, ZeroVector, volume, false);
	}
} 
