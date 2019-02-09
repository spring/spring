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

	bool HasSoundItem(const std::string& name) const override { return false; }
	bool PreloadSoundItem(const std::string& name) override { return false; }
	size_t GetDefSoundId(const std::string& name) override { return 0; }
	size_t GetSoundId(const std::string& name) override { return 0; }

	SoundItem* GetSoundItem(size_t id) override { return nullptr; }
	CSoundSource* GetNextBestSource(bool lock = true) override { return nullptr; }

	void UpdateListener(const float3& campos, const float3& camdir, const float3& camup) override {}
	void NewFrame() override {}

	void ConfigNotify(const std::string& key, const std::string& value) override {}
	void PitchAdjust(const float newPitch) override {}

	bool Mute() override { return true; }
	bool IsMuted() const override { return true; }

	void Iconified(bool state) override {}

	void PrintDebugInfo() override;
	bool SoundThreadQuit() const override { return false; }
	bool CanLoadSoundDefs() const override { return true; }
	bool LoadSoundDefsImpl(LuaParser* defsParser) { return false; }

	const float3& GetListenerPos() const override { return ZeroVector; }
};

#endif // _NULL_SOUND_H_
