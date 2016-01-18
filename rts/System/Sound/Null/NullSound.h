/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _NULL_SOUND_H_
#define _NULL_SOUND_H_

#include "System/Sound/ISound.h"
#include "System/float3.h"

#include <string>

class float3;
class CSoundSource;

/// Empty sound system implementation
class NullSound : public ISound
{
public:
	NullSound() {}
	virtual ~NullSound() {}

	bool HasSoundItem(const std::string& name) const { return false; }
	size_t GetSoundId(const std::string& name) { return 0; }
	SoundItem* GetSoundItem(size_t id) const { return NULL; }

	CSoundSource* GetNextBestSource(bool lock = true) { return NULL; }

	void UpdateListener(const float3& campos, const float3& camdir, const float3& camup) {}
	void NewFrame() {}

	void ConfigNotify(const std::string& key, const std::string& value) {}
	void PitchAdjust(const float newPitch) {}

	bool Mute() { return true; }
	bool IsMuted() const { return true; }

	void Iconified(bool state) {}

	void PrintDebugInfo();
	bool SoundThreadQuit() const { return false; }
	bool CanLoadSoundDefs() const { return true; }
	bool LoadSoundDefsImpl(const std::string& fileName, const std::string& modes) { return false; }

	const float3& GetListenerPos() const { return ZeroVector; }
};

#endif // _NULL_SOUND_H_
