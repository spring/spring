/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _EFX_H_
#define _EFX_H_

#include <string>

#include <al.h>
#include <alc.h>
#include <efx.h>

#include "EFXPresets.h"
#include "System/UnorderedMap.hpp"

/// Default sound effects system implementation
class CEFX
{
public:
	void Init(ALCdevice* device);
	void Kill();
	void ResetState() {
		updates = 0;
		maxSlots = 0;

		enabled = false;
		supported = false;

		sfxProperties = {};

		sfxSlot = 0;
		sfxReverb = 0;
		sfxFilter = 0;
		maxSlotsPerSource = 0;

		airAbsorptionFactor = 0.0f;
		heightRolloffModifier = 1.0f;

		effectsSupported.clear();
		filtersSupported.clear();
	}

	void SetPreset(const std::string& name, bool verbose = true, bool commit = true);
	void CommitEffects(const EAXSfxProps* sfxProps = nullptr);

	void Enable();
	void Disable();

	void SetHeightRolloffModifer(float mod);

	bool Enabled() const { return enabled; }
	bool Supported() const { return supported; }

public:
	/// @see ConfigHandler::ConfigNotifyCallback
	void ConfigNotify(const std::string& key, const std::string& value);

	void SetAirAbsorptionFactor(ALfloat value);
	ALfloat GetAirAbsorptionFactor() const { return airAbsorptionFactor; }

public:
	int updates = 0;
	int maxSlots = 0;

	bool enabled = false;
	bool supported = false;

	EAXSfxProps sfxProperties;

	ALuint sfxSlot = 0;
	ALuint sfxReverb = 0;
	ALuint sfxFilter = 0;
	ALuint maxSlotsPerSource = 0;

private:
	ALfloat airAbsorptionFactor = 0.0f;

	// reduces the rolloff when camera is high above the ground (so we still hear something in tab mode or far zoom)
	float heightRolloffModifier = 1.0f;

private:
	// information about the supported features
	spring::unsynced_map<ALuint, bool> effectsSupported;
	spring::unsynced_map<ALuint, bool> filtersSupported;
};

// initialized in Sound.cpp
extern CEFX efx;

#endif // _EFX_H_
