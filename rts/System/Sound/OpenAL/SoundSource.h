/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SOUNDSOURCE_H
#define SOUNDSOURCE_H

#include <string>

#include <al.h>
#include "System/Misc/NonCopyable.h"
#include "System/Misc/SpringTime.h"
#include "System/float3.h"

class IAudioChannel;
class SoundItem;
class COggStream;

/**
 * @brief One soundsource wich can play some sounds
 *
 * Construct some of them, and they can play SoundItems positioned anywhere in 3D-space for you.
 */
class CSoundSource : spring::noncopyable
{
public:
	/// is ready after this
	CSoundSource();
	/// will stop during deletion
	~CSoundSource();

	void Update();

	void UpdateVolume();
	bool IsValid() const { return (id != 0); };

	int GetCurrentPriority() const;
	bool IsPlaying(const bool checkOpenAl = false) const;
	void Stop();

	/// will stop a currently playing sound, if any
	void Play(IAudioChannel* channel, SoundItem* buffer, float3 pos, float3 velocity, float volume, bool relative = false);
	void PlayAsync(IAudioChannel* channel, SoundItem* buffer, float3 pos, float3 velocity, float volume, bool relative = false);
	void PlayStream(IAudioChannel* channel, const std::string& stream, float volume);
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
	struct AsyncSoundItemData {
		IAudioChannel* channel;
		SoundItem* buffer;
		float3 pos;
		float3 velocity;
		float volume;
		bool relative;

		AsyncSoundItemData()
		: channel(nullptr)
		, buffer(nullptr)
		, volume(1.0f)
		, relative(false)
		{}
	};

	static float referenceDistance;

	//! used to adjust the pitch to the GameSpeed (optional)
	static float globalPitch;

	//! reduce the rolloff when the camera is height above the ground (so we still hear something in tab mode or far zoom)
	static float heightRolloffModifier;

	ALuint id;
	SoundItem* curPlaying;
	IAudioChannel* curChannel;
	COggStream* curStream;
	float curVolume;
	spring_time loopStop;
	bool in3D;
	bool efxEnabled;
	int efxUpdates;
	ALfloat curHeightRolloffModifier;
	AsyncSoundItemData asyncPlay;
};

#endif
