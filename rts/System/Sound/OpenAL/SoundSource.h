/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SOUNDSOURCE_H
#define SOUNDSOURCE_H

#include <string>

#include <al.h>

#include "System/Misc/NonCopyable.h"
#include "System/Misc/SpringTime.h"
#include "System/float3.h"
#include "OggStream.h"

class IAudioChannel;
class SoundItem;

/**
 * @brief One soundsource wich can play some sounds
 *
 * Construct some of them, and they can play SoundItems positioned anywhere in 3D-space for you.
 */
class CSoundSource
{
public:
	/// is ready after this
	CSoundSource();
	CSoundSource(CSoundSource&& src) { *this = std::move(src); }
	CSoundSource(const CSoundSource& src) = delete;
	~CSoundSource() { Delete(); }

	// sources don't ever need to actually move, just here to satisfy compiler
	CSoundSource& operator = (CSoundSource&& src) { return *this; }
	CSoundSource& operator = (const CSoundSource& src) = delete;

	void Update();
	void Delete();

	void UpdateVolume();
	bool IsValid() const { return (id != 0); };

	int GetCurrentPriority() const;
	bool IsPlaying(const bool checkOpenAl = false) const;
	void Stop();

	/// will stop a currently playing sound, if any
	void Play(IAudioChannel* channel, SoundItem* item, float3 pos, float3 velocity, float volume, bool relative = false);
	void PlayAsync(IAudioChannel* channel, size_t id, float3 pos, float3 velocity, float volume, float priority, bool relative = false);
	void PlayStream(IAudioChannel* channel, const std::string& stream, float volume);
	void StreamStop();
	void StreamPause();
	float GetStreamTime();
	float GetStreamPlayTime();

	static void SetPitch(const float& newPitch) { globalPitch = newPitch; }
	static void SetHeightRolloffModifer(const float& mod) { heightRolloffModifier = mod; }

private:
	struct AsyncSoundItemData {
		IAudioChannel* channel = nullptr;

		size_t id = 0;

		float3 position;
		float3 velocity;

		float volume = 1.0f;
		float priority = 0.0f;

		bool relative = false;
	};

	// light-weight SoundItem with only the data needed for playback
	struct SoundItemData {
		size_t id;

		unsigned int loopTime;
		int priority;

		float rndGain;
		float rolloff;
	};

private:
	// used to adjust the pitch to the GameSpeed (optional)
	static float globalPitch;

	// reduce the rolloff when the camera is height above the ground (so we still hear something in tab mode or far zoom)
	static float heightRolloffModifier;

private:
	ALuint id;

	SoundItemData curPlayingItem;
	AsyncSoundItemData asyncPlayItem;

	IAudioChannel* curChannel;
	COggStream curStream;

	float curVolume;
	spring_time loopStop;
	bool in3D;
	bool efxEnabled;
	int efxUpdates;

	ALfloat curHeightRolloffModifier;
};

#endif
