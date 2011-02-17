/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _NULL_SOUND_H_
#define _NULL_SOUND_H_

#include "ISound.h"

#include <string>

class float3;
class SoundSource;

/// Empty sound system implementation
class NullSound : public ISound
{
public:
	NullSound();
	virtual ~NullSound();

	virtual bool HasSoundItem(const std::string& name);
	virtual size_t GetSoundId(const std::string& name, bool hardFail = true);

	virtual SoundSource* GetNextBestSource(bool lock = true);

	virtual void UpdateListener(const float3& campos, const float3& camdir, const float3& camup, float lastFrameTime);
	virtual void NewFrame();

	virtual void ConfigNotify(const std::string& key, const std::string& value);
	virtual void PitchAdjust(const float newPitch);

	virtual bool Mute();
	virtual bool IsMuted() const;

	virtual void Iconified(bool state);

	virtual void PrintDebugInfo();
	virtual bool LoadSoundDefs(const std::string& fileName);

private:
	friend class EffectChannel;
	// this is used by EffectChannel in AudioChannel.cpp
	virtual void PlaySample(size_t id, const float3 &p, const float3& velocity, float volume, bool relative);
};

#endif // _NULL_SOUND_H_
