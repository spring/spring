/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _I_SOUND_H_
#define _I_SOUND_H_

#include <string>

class float3;
class LuaParser;
class CSoundSource;
class SoundItem;


/**
 * @brief sound system interface
 * This is used in other parts of the engine, whenever sound should be played.
 */
class ISound {
	static ISound* singleton;

public:
	static void Initialize(bool reload, bool forceNullSound = false);
	static void Shutdown(bool reload);
	static bool IsInitialized() { return (singleton != nullptr); }
	static inline ISound* GetInstance() { return singleton; }


	virtual ~ISound() {}
	virtual void Init() {}
	virtual void Kill() {}

	virtual bool HasSoundItem(const std::string& name) const = 0;
	virtual bool PreloadSoundItem(const std::string& name) = 0;
	virtual size_t GetDefSoundId(const std::string& name) = 0;
	virtual size_t GetSoundId(const std::string& name) = 0;


	virtual SoundItem* GetSoundItem(size_t id) = 0;
	/**
	 * Returns a free sound source if available,
	 * the one with the lowest priority otherwise.
	 */
	virtual CSoundSource* GetNextBestSource(bool lock = true) = 0;


	virtual void UpdateListener(const float3& camPos, const float3& camDir, const float3& camUp) = 0;
	virtual void NewFrame() = 0;

	virtual void ConfigNotify(const std::string& key, const std::string& value) = 0;
	virtual void PitchAdjust(const float newPitch) = 0;

	/// @return true if now muted, false otherwise
	virtual bool Mute() = 0;
	virtual bool IsMuted() const = 0;

	///change current output device
	static bool ChangeOutput(bool forceNullSound = false);

	virtual void Iconified(bool state) = 0;

	virtual void PrintDebugInfo() = 0;

	virtual bool SoundThreadQuit() const = 0;
	virtual bool CanLoadSoundDefs() const = 0;

	bool LoadSoundDefs(LuaParser* defsParser);

	virtual const float3& GetListenerPos() const = 0;

public:
	unsigned numEmptyPlayRequests = 0;
	unsigned numAbortedPlays = 0;

private:
	virtual bool LoadSoundDefsImpl(LuaParser* defsParser) = 0;
	static bool IsNullAudio();
};

#define sound ISound::GetInstance()

#endif // _I_SOUND_H_
