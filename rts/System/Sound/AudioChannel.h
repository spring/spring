#ifndef AUDIO_CHANNEL_H
#define AUDIO_CHANNEL_H

#include "float3.h"

class CWorldObject;
class CUnit;

/**
 * @brief Channel for playing sounds
 * 
 * Has its own volume "slider", and can be enabled / disabled seperately
 */
class AudioChannel
{
public:
	AudioChannel();

	void Enable(bool newState)
	{
		enabled = newState;
	};
	bool IsEnabled()
	{
		return enabled;
	};

	void SetVolume(float newVolume)
	{
		volume = newVolume;
	};
	float GetVolume()
	{
		return volume;
	};

	void PlaySample(size_t id, float volume = 1.0f);
	void PlaySample(size_t id, const float3& p, float volume = 1.0f);
	void PlaySample(size_t id, const float3& p, const float3& velocity, float volume = 1.0f);

	void PlaySample(size_t id, CUnit* u, float volume = 1.0f);
	void PlaySample(size_t id, CWorldObject* p, float volume = 1.0f);

	void UpdateFrame()
	{
		emmitsThisFrame = 0;
	};
	void SetMaxEmmits(unsigned max)
	{
		emmitsPerFrame = max;
	};

private:
	float volume;
	bool enabled;
	unsigned emmitsPerFrame;
	unsigned emmitsThisFrame;
};

/**
 * @brief If you want to play a sound, use one of these
 */
namespace Channels
{
	extern AudioChannel General;
	extern AudioChannel Battle;
	extern AudioChannel UnitReply;
	extern AudioChannel UserInterface;
}

#endif
