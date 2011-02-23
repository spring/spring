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
class CSoundSource : boost::noncopyable
{
public:
	/// is ready after this
	CSoundSource();
	/// will stop during deletion
	~CSoundSource();

	void Update();

	void SetVolume(float newVol);
	bool IsValid() const { return (id != 0); };

	int GetCurrentPriority() const;
	bool IsPlaying() const;
	void Stop();

	/// will stop a currently playing sound, if any
	void Play(SoundItem* buffer, float3 pos, float3 velocity, float volume, bool relative = false);
	void PlayStream(const std::string& stream, float volume, bool enqueue);
	void StreamStop();
	void StreamPause();
	float GetStreamTime();
	float GetStreamPlayTime();

	static void SetPitch(const float& newPitch)
	{
		globalPitch = newPitch;
	};
	static void SetHeightRolloffModifer(const float& mod)
	{
		heightRolloffModifier = mod;
	};

private:
	static float referenceDistance;

	//! used to adjust the pitch to the GameSpeed (optional)
	static float globalPitch;

	//! reduce the rolloff when the camera is height above the ground (so we still hear something in tab mode or far zoom)
	static float heightRolloffModifier;

	ALuint id;
	SoundItem* curPlaying;
	struct StreamControl;
	StreamControl* curStream;
	boost::mutex streamMutex;
	unsigned loopStop;
	bool in3D;
	bool efxEnabled;
	ALfloat curHeightRolloffModifier;
};

#endif
