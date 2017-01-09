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
	CEFX(ALCdevice* device);
	~CEFX();

	void SetPreset(const std::string& name, bool verbose = true, bool commit = true);
	void CommitEffects();

	void Enable();
	void Disable();

	void SetHeightRolloffModifer(const float& mod);

public:
	/// @see ConfigHandler::ConfigNotifyCallback
	void ConfigNotify(const std::string& key, const std::string& value);

	void SetAirAbsorptionFactor(ALfloat value);
	ALfloat GetAirAbsorptionFactor() const { return airAbsorptionFactor; }


	int updates;
	int maxSlots;

	bool enabled;
	bool supported;

	EAXSfxProps sfxProperties;

	ALuint sfxSlot;
	ALuint sfxReverb;
	ALuint sfxFilter;
	ALuint maxSlotsPerSource;

private:
	ALfloat airAbsorptionFactor;

	// reduces the rolloff when camera is high above the ground (so we still hear something in tab mode or far zoom)
	static float heightRolloffModifier;

private:
	// information about the supported features
	spring::unordered_map<ALuint, bool> effectsSupported;
	spring::unordered_map<ALuint, bool> filtersSupported;
};

//! init in Sound.cpp
extern CEFX* efx;

#endif // _EFX_H_
