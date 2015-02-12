/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "EFX.h"

#include "ALShared.h"
#include "EFXPresets.h"
#include "EFXfuncs.h"

#include "System/Sound/SoundLog.h"
#include "System/Config/ConfigHandler.h"
#include "System/myMath.h"


/******************************************************************************/
/******************************************************************************/

static std::string default_preset = "outdoors_valley";//"bathroom";

/******************************************************************************/
/******************************************************************************/

CEFX* efx = NULL;
float CEFX::heightRolloffModifier = 1.f;

CEFX::CEFX(ALCdevice* device)
	:enabled(false)
	,supported(false)
	,sfxProperties(NULL)
	,sfxSlot(0)
	,sfxReverb(0)
	,sfxFilter(0)
	,updates(0)
	,maxSlots(0)
	,maxSlotsPerSource(0)
{
	SetAirAbsorptionFactor(configHandler->GetFloat("snd_airAbsorption"));

	bool hasExtension = alcIsExtensionPresent(device, "ALC_EXT_EFX");

	if(hasExtension && alGenEffects && alDeleteEffects)
		supported = true;

	//! set default preset
	eaxPresets["default"] = eaxPresets[default_preset];

	//! always allocate this
	sfxProperties = new EAXSfxProps();
	*sfxProperties = eaxPresets[default_preset];

	if (!supported) {
		if(!hasExtension) {
			LOG("  EFX Supported: no");
		} else {
			LOG("  EFX is supported but software does not seem to work properly");
		}
		return;
	}

	//! clear log
	alGetError() ;

	//! Check Available Effects
	{
		static const ALuint effects[] = {
			AL_EFFECT_REVERB,
			AL_EFFECT_EAXREVERB,
			AL_EFFECT_CHORUS,
			AL_EFFECT_DISTORTION,
			AL_EFFECT_ECHO,
			AL_EFFECT_FLANGER,
			AL_EFFECT_FREQUENCY_SHIFTER,
			AL_EFFECT_VOCAL_MORPHER,
			AL_EFFECT_PITCH_SHIFTER,
			AL_EFFECT_RING_MODULATOR,
			AL_EFFECT_AUTOWAH,
			AL_EFFECT_COMPRESSOR,
			AL_EFFECT_EQUALIZER
		};

		ALuint alFx;
		alGenEffects(1, &alFx);
		if (alGetError() == AL_NO_ERROR) {
			for(size_t i = 0; i < sizeof(effects)/sizeof(effects[0]); i++) {
				const ALuint fx = effects[i];
				alEffecti(alFx, AL_EFFECT_TYPE, fx);
				effectsSupported[fx] = (alGetError() == AL_NO_ERROR);
			}
		}
		alDeleteEffects(1, &alFx);
	}

	//! Check Available Filters
	{
		static const ALuint filters[] = {
			AL_FILTER_LOWPASS,
			AL_FILTER_HIGHPASS,
			AL_FILTER_BANDPASS
		};

		ALuint alFilter;
		alGenFilters(1, &alFilter);
		if (alGetError() == AL_NO_ERROR) {
			for(size_t i = 0; i < sizeof(filters)/sizeof(filters[0]); i++) {
				const ALuint filter = filters[i];
				alFilteri(alFilter, AL_FILTER_TYPE, filter);
				filtersSupported[filter] = (alGetError() == AL_NO_ERROR);
			}
		}
		alDeleteFilters(1, &alFilter);
	}

	//! Check Max Available EffectSlots
	{
		int n;
		ALuint alFXSlots[128];
		for (n = 0; n < 128; n++) {
			alGenAuxiliaryEffectSlots(1, &alFXSlots[n]);
			if (alGetError() != AL_NO_ERROR)
				break;
		}
		maxSlots = n;

		alDeleteAuxiliaryEffectSlots(n, alFXSlots);
	}

	//! Check Max AUX FX SLOTS Per Sound Source
	alcGetIntegerv(device, ALC_MAX_AUXILIARY_SENDS, 1, (ALCint*)&maxSlotsPerSource);


	//! Check requirements
	if (!effectsSupported[AL_EFFECT_EAXREVERB]
		|| !filtersSupported[AL_FILTER_LOWPASS]
		|| (maxSlots<1)
		|| (maxSlotsPerSource<1)
	) {
		if (enabled) {
			LOG_L(L_WARNING, "  EFX Supported: no");
		}
		supported = false;
		return;
	}


	//! Create our global sfx enviroment
	alGenAuxiliaryEffectSlots(1, &sfxSlot);
	alGenEffects(1, &sfxReverb);
		alEffecti(sfxReverb, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);
	alGenFilters(1, &sfxFilter);
		alFilteri(sfxFilter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
	if (!alIsAuxiliaryEffectSlot(sfxSlot) || !alIsEffect(sfxReverb) || !alIsFilter(sfxFilter)) {
		LOG_L(L_ERROR, "  Initializing EFX failed!");
		alDeleteFilters(1, &sfxFilter);
		alDeleteEffects(1, &sfxReverb);
		alDeleteAuxiliaryEffectSlots(1, &sfxSlot);
		supported = false;
		return;
	}


	//! Load defaults
	CommitEffects();
	if (!CheckError("  EFX")) {
		LOG_L(L_ERROR, "  Initializing EFX failed!");
		alAuxiliaryEffectSloti(sfxSlot, AL_EFFECTSLOT_EFFECT, AL_EFFECT_NULL);
		alDeleteFilters(1, &sfxFilter);
		alDeleteEffects(1, &sfxReverb);
		alDeleteAuxiliaryEffectSlots(1, &sfxSlot);
		supported = false;
		return;
	}

	//! User may disable it (performance reasons?)
	enabled = configHandler->GetBool("UseEFX");
	LOG("  EFX Enabled: %s", (enabled ? "yes" : "no"));
	if (enabled) {
		LOG_L(L_DEBUG, "  EFX MaxSlots: %i", maxSlots);
		LOG_L(L_DEBUG, "  EFX MaxSlotsPerSource: %i", maxSlotsPerSource);
	}

	configHandler->NotifyOnChange(this);
}


CEFX::~CEFX()
{
	configHandler->RemoveObserver(this);
	if (supported) {
		alAuxiliaryEffectSloti(sfxSlot, AL_EFFECTSLOT_EFFECT, AL_EFFECT_NULL);
		alDeleteFilters(1, &sfxFilter);
		alDeleteEffects(1, &sfxReverb);
		alDeleteAuxiliaryEffectSlots(1, &sfxSlot);
	}
	delete sfxProperties;
}


void CEFX::Enable()
{
	if (supported && !enabled) {
		enabled = true;
		CommitEffects();
		LOG("EAX enabled");
	}
}


void CEFX::Disable()
{
	if (enabled) {
		enabled = false;
		alAuxiliaryEffectSloti(sfxSlot, AL_EFFECTSLOT_EFFECT, AL_EFFECT_NULL);
		LOG("EAX disabled");
	}
}


void CEFX::SetPreset(std::string name, bool verbose, bool commit)
{
	if (!supported)
		return;


	std::map<std::string, EAXSfxProps>::const_iterator it = eaxPresets.find(name);
	if (it != eaxPresets.end()) {
		*sfxProperties = it->second;
		if (commit)
			CommitEffects();
		if (verbose)
			LOG("EAX Preset changed to: %s", name.c_str());
	}
}


void CEFX::SetHeightRolloffModifer(const float& mod)
{
	heightRolloffModifier = mod;

	if (!supported)
		return;

	alEffectf(sfxReverb, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, sfxProperties->properties_f[AL_EAXREVERB_ROOM_ROLLOFF_FACTOR] * heightRolloffModifier);
	alAuxiliaryEffectSloti(sfxSlot, AL_EFFECTSLOT_EFFECT, sfxReverb);
}


void CEFX::CommitEffects()
{
	if (!supported)
		return;

	//! commit reverb properties
	for (std::map<ALuint, ALfloat>::iterator it = sfxProperties->properties_f.begin(); it != sfxProperties->properties_f.end(); ++it)
		alEffectf(sfxReverb, it->first, it->second);
	for (std::map<ALuint, ALint>::iterator it = sfxProperties->properties_i.begin(); it != sfxProperties->properties_i.end(); ++it)
		alEffecti(sfxReverb, it->first, it->second);
	for (std::map<ALuint, float3>::iterator it = sfxProperties->properties_v.begin(); it != sfxProperties->properties_v.end(); ++it)
		alEffectfv(sfxReverb, it->first, (ALfloat*)&it->second[0]);

	alEffectf(sfxReverb, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, sfxProperties->properties_f[AL_EAXREVERB_ROOM_ROLLOFF_FACTOR] * heightRolloffModifier);
	alAuxiliaryEffectSloti(sfxSlot, AL_EFFECTSLOT_EFFECT, sfxReverb);

	for (std::map<ALuint, ALfloat>::iterator it=sfxProperties->filter_properties_f.begin(); it != sfxProperties->filter_properties_f.end(); ++it)
		alFilterf(sfxFilter, it->first, it->second);

	updates++;
}

void CEFX::SetAirAbsorptionFactor(ALfloat value)
{
	airAbsorptionFactor = Clamp(value, AL_MIN_AIR_ABSORPTION_FACTOR, AL_MAX_AIR_ABSORPTION_FACTOR);
}

void CEFX::ConfigNotify(const std::string& key, const std::string& value)
{
	if (key == "snd_airAbsorption") {
		SetAirAbsorptionFactor(std::atof(value.c_str()));
	}
}
