/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _I_SOUND_H_
#define _I_SOUND_H_

#include <string>

class float3;
class SoundSource;


/**
 * @brief sound system interface
 * This is used in other parts of the engine, whenever sound should be played.
 */
class ISound {
	static ISound* singleton;

public:
	virtual ~ISound() {};

	static void Initialize();
	static void Shutdown();
	static bool IsInitialized();
	static inline ISound* GetInstance() {
		return singleton;
	}

	virtual bool HasSoundItem(const std::string& name) = 0;
	virtual size_t GetSoundId(const std::string& name, bool hardFail = true) = 0;

	/**
	 * Returns a free sound source if available,
	 * the one with the lowest priority otherwise.
	 */
	virtual SoundSource* GetNextBestSource(bool lock = true) = 0;

	virtual void UpdateListener(const float3& camPos, const float3& camDir, const float3& camUp, float lastFrameTime) = 0;
	virtual void NewFrame() = 0;

	virtual void ConfigNotify(const std::string& key, const std::string& value) = 0;
	virtual void PitchAdjust(const float newPitch) = 0;

	/// @return true if now muted, false otherwise
	virtual bool Mute() = 0;
	virtual bool IsMuted() const = 0;

	virtual void Iconified(bool state) = 0;

	virtual void PrintDebugInfo() = 0;
	virtual bool LoadSoundDefs(const std::string& fileName) = 0;

private:
	friend class EffectChannel;
	// this is used by EffectChannel in AudioChannel.cpp
	virtual void PlaySample(size_t id, const float3 &p, const float3& velocity, float volume, bool relative) = 0;
};

#define sound ISound::GetInstance()

#endif // _I_SOUND_H_
