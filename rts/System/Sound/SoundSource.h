#ifndef SOUNDSOURCE_H
#define SOUNDSOURCE_H

#include <string>

#include <al.h>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>

class float3;
class SoundItem;
class COggStream;

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
	void PlayStream(const std::string& stream, float volume);
	void StreamStop();

	void StreamPause();
	unsigned GetStreamTime();
	unsigned GetStreamPlayTime();

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
};

#endif
