/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _NULL_SOUND_H_
#define _NULL_SOUND_H_

#include "ISound.h"

#include <string>

class float3;
class CSoundSource;

/// Empty sound system implementation
class NullSound : public ISound
{
public:
	NullSound();
	virtual ~NullSound();

	bool HasSoundItem(const std::string& name) const;
	size_t GetSoundId(const std::string& name);
	SoundItem* GetSoundItem(size_t id) const { return NULL; }

	CSoundSource* GetNextBestSource(bool lock = true);

	void UpdateListener(const float3& campos, const float3& camdir, const float3& camup, float lastFrameTime);
	void NewFrame();

	void ConfigNotify(const std::string& key, const std::string& value);
	void PitchAdjust(const float newPitch);

	bool Mute();
	bool IsMuted() const;

	void Iconified(bool state);

	void PrintDebugInfo();
	bool LoadSoundDefs(const std::string& fileName);
	
	const float3& GetListenerPos() const;
};

#endif // _NULL_SOUND_H_
