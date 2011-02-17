/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SOUNDSOURCE_H
#define SOUNDSOURCE_H

#include <string>

#include <al.h>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>

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
	void PlayStream(const std::string& stream, float volume, bool enqueue);
	void StreamStop();

	void StreamPause();
	float GetStreamTime();
	float GetStreamPlayTime();

	static void SetPitch(float newPitch);
	void SetVolume(float newVol);
	bool IsValid() const
	{
		return (id != 0);
	};

	static void SetHeightRolloffModifer(float mod)
	{
		heightAdjustedRolloffModifier = mod > 0.0f ? mod : 0.0f;
	};

	static void SetAirAbsorptionSupported(bool supported);
	/**
	 * Sets the amount of air absorption.
	 * Air absorption filters out high-frequency sounds, relative to distance.
	 * Higher value -> filter out a lot on small distances already.
	 * @param factor from 0.0f (disabled) till 10.0f
	 */
	static void SetAirAbsorption(float factor = 0.0f);

private:
	/// pitch shared by all sources
	static float globalPitch;
	ALuint id;
	SoundItem* curPlaying;

	struct StreamControl;
	StreamControl* curStream;
	boost::mutex streamMutex;
	unsigned loopStop;
	static float heightAdjustedRolloffModifier;
	static float referenceDistance;
	static float airAbsorption;
	static bool airAbsorptionSupported;
};

#endif
