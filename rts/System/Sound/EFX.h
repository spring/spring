/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _EFX_H_
#define _EFX_H_

#include <string>
#include <map>
#include <al.h>
#include <alc.h>
#include <efx.h>

struct EAXSfxProps;

/// Default sound effects system implementation
class CEFX
{
public:
	CEFX(ALCdevice* device);
	~CEFX();

	void SetPreset(std::string name, bool verbose = true, bool commit = true);
	void CommitEffects();

	void Enable();
	void Disable();

	void SetHeightRolloffModifer(const float& mod);

public:
	/// @see ConfigHandler::ConfigNotifyCallback
	void ConfigNotify(const std::string& key, const std::string& value);

	void SetAirAbsorptionFactor(ALfloat value);
	ALfloat GetAirAbsorptionFactor() const { return airAbsorptionFactor; }


	bool enabled;
	bool supported;

	EAXSfxProps* sfxProperties;
	ALuint sfxSlot;
	ALuint sfxReverb;
	ALuint sfxFilter;
private:
	ALfloat airAbsorptionFactor;
public:

	int updates;

private:
	//! reduce the rolloff when the camera is height above the ground (so we still hear something in tab mode or far zoom)
	static float heightRolloffModifier;

private:
	//! some more information about the supported features
	std::map<ALuint, bool> effectsSupported;
	std::map<ALuint, bool> filtersSupported;
	int maxSlots;
	ALuint maxSlotsPerSource;
};

//! init in Sound.cpp
extern CEFX* efx;

#endif // _EFX_H_
