#ifndef SOUNDSOURCE_H
#define SOUNDSOURCE_H

#include <AL/al.h>
#include <boost/noncopyable.hpp>

class float3;
class SoundItem;

/**
 * @brief One soundsource wich can play some sounds
 * 
 * Construct some of them, and they can play SoundItems positioned anywhere in 3D-space for you.
 */
class SoundSource : boost::noncopyable
{
public:
	/// is ready after this
	SoundSource();
	/// will stop during deletion
	~SoundSource();

	void Update();

	int GetCurrentPriority() const;
	bool IsPlaying() const;
	void Stop();
	/// will stop a currently playing sound, if any
	void Play(SoundItem* buffer, const float3& pos, float3 velocity, float volume, bool relative = false);
	static void SetPitch(float newPitch);
	bool IsValid() const
	{
		return (id != 0);
	};

	static void SetHeightRolloffModifer(float mod)
	{
		heightAdjustedRolloffModifier = mod > 0.0f ? mod : 0.0f;
	};

private:
	/// pitch shared by all sources
	static float globalPitch;
	ALuint id;
	SoundItem* curPlaying;
	unsigned loopStop;
	static float heightAdjustedRolloffModifier;
};

#endif
