#include "AudioChannel.h"

#include "Sound.h"
#include "Sim/Units/Unit.h"

namespace Channels
{
	AudioChannel General;
	AudioChannel Battle;
	AudioChannel UnitReply;
	AudioChannel UserInterface;
}

AudioChannel::AudioChannel() : volume(1.0f), enabled(true)
{
};

void AudioChannel::PlaySample(size_t id, float ovolume)
{
	if (enabled)
		sound->PlaySample(id, ZeroVector, ZeroVector, volume * ovolume, true);
}

void AudioChannel::PlaySample(size_t id, const float3& p, float ovolume)
{
	if (enabled)
		sound->PlaySample(id, p, ZeroVector, volume * ovolume, false);
}

void AudioChannel::PlaySample(size_t id, const float3& p, const float3& velocity, float ovolume)
{
	if (enabled)
		sound->PlaySample(id, p, velocity, volume * ovolume, false);
}

void AudioChannel::PlaySample(size_t id, CUnit* u,float ovolume)
{
	if (enabled)
		sound->PlaySample(id, u->pos, u->speed, volume * ovolume, false);
}

void AudioChannel::PlaySample(size_t id, CWorldObject* p,float ovolume)
{
	if (enabled)
		sound->PlaySample(id, p->pos, ZeroVector, volume * ovolume, false);
} 
