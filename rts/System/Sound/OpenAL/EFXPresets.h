/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _EFX_PRESETS_H_
#define _EFX_PRESETS_H_

#include <string>

#include <al.h>
#include <efx.h>

#include "System/float3.h"
#include "System/UnorderedMap.hpp"

struct EAXSfxProps {
	EAXSfxProps() {}
	EAXSfxProps(
		ALfloat _density,
		ALfloat _diffusion,
		ALfloat _gain,
		ALfloat _gainHF,
		ALfloat _gainLF,
		ALfloat _decayTime,
		ALfloat _decayHFRatio,
		ALfloat _decayLFRatio,
		ALfloat _reflectionGain,
		ALfloat _reflectionDelay,
		float3 _reflectionPan,
		ALfloat _lateReverbGain,
		ALfloat _lateReverbDelay,
		float3 _lateReverbPan,
		ALfloat _echoTime,
		ALfloat _echoDepth,
		ALfloat _modTime,
		ALfloat _modDepth,
		ALfloat _airAbsorptionGainHF,
		ALfloat _hfReference,
		ALfloat _lfReference,
		ALfloat _roomRollOffFactor,
		ALboolean _decayHFLimit
	) {
		reverb_props_f[AL_EAXREVERB_DENSITY] = _density;
		reverb_props_f[AL_EAXREVERB_DIFFUSION] = _diffusion;
		reverb_props_f[AL_EAXREVERB_GAIN] = _gain;
		reverb_props_f[AL_EAXREVERB_GAINHF] = _gainHF;
		reverb_props_f[AL_EAXREVERB_GAINLF] = _gainLF;
		reverb_props_f[AL_EAXREVERB_DECAY_TIME] = _decayTime;
		reverb_props_i[AL_EAXREVERB_DECAY_HFLIMIT] = _decayHFLimit;
		reverb_props_f[AL_EAXREVERB_DECAY_HFRATIO] = _decayHFRatio;
		reverb_props_f[AL_EAXREVERB_DECAY_LFRATIO] = _decayLFRatio;
		reverb_props_f[AL_EAXREVERB_REFLECTIONS_GAIN] = _reflectionGain;
		reverb_props_f[AL_EAXREVERB_REFLECTIONS_DELAY] = _reflectionDelay;
		reverb_props_v[AL_EAXREVERB_REFLECTIONS_PAN] = _reflectionPan;
		reverb_props_f[AL_EAXREVERB_LATE_REVERB_GAIN] = _lateReverbGain;
		reverb_props_f[AL_EAXREVERB_LATE_REVERB_DELAY] = _lateReverbDelay;
		reverb_props_v[AL_EAXREVERB_LATE_REVERB_PAN] = _lateReverbPan;
		reverb_props_f[AL_EAXREVERB_ECHO_TIME] = _echoTime;
		reverb_props_f[AL_EAXREVERB_ECHO_DEPTH] = _echoDepth;
		reverb_props_f[AL_EAXREVERB_MODULATION_TIME] = _modTime;
		reverb_props_f[AL_EAXREVERB_MODULATION_DEPTH] = _modDepth;
		reverb_props_f[AL_EAXREVERB_AIR_ABSORPTION_GAINHF] = _airAbsorptionGainHF;
		reverb_props_f[AL_EAXREVERB_HFREFERENCE] = _hfReference;
		reverb_props_f[AL_EAXREVERB_LFREFERENCE] = _lfReference;
		reverb_props_f[AL_EAXREVERB_ROOM_ROLLOFF_FACTOR] = _roomRollOffFactor;

		filter_props_f[AL_LOWPASS_GAIN] = 1.0f;
		filter_props_f[AL_LOWPASS_GAINHF] = 1.0f;
	}

	spring::unsynced_map<ALuint, ALfloat> reverb_props_f;
	spring::unsynced_map<ALuint, ALint>   reverb_props_i;
	spring::unsynced_map<ALuint, float3>  reverb_props_v;
	spring::unsynced_map<ALuint, ALfloat> filter_props_f;
};

namespace EFXParamTypes {
	enum {
		FLOAT,
		VECTOR,
		BOOL
	};
}

extern spring::unsynced_map<std::string, EAXSfxProps> eaxPresets;

extern spring::unsynced_map<ALuint, unsigned> alParamType;
extern spring::unsynced_map<std::string, ALuint> nameToALParam;
extern spring::unsynced_map<ALuint, std::string> alParamToName;
extern spring::unsynced_map<std::string, ALuint> nameToALFilterParam;
extern spring::unsynced_map<ALuint, std::string> alFilterParamToName;

#endif // _EFX_PRESETS_H_
