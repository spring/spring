/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _EFX_PRESETS_H_
#define _EFX_PRESETS_H_

#include <string>
#include <map>
#include "System/float3.h"
#include <al.h>
#include <efx.h>


struct EAXSfxProps
{
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
	)
	{
		properties_f[AL_EAXREVERB_DENSITY] = _density;
		properties_f[AL_EAXREVERB_DIFFUSION] = _diffusion;
		properties_f[AL_EAXREVERB_GAIN] = _gain;
		properties_f[AL_EAXREVERB_GAINHF] = _gainHF;
		properties_f[AL_EAXREVERB_GAINLF] = _gainLF;
		properties_f[AL_EAXREVERB_DECAY_TIME] = _decayTime;
		properties_i[AL_EAXREVERB_DECAY_HFLIMIT] = _decayHFLimit;
		properties_f[AL_EAXREVERB_DECAY_HFRATIO] = _decayHFRatio;
		properties_f[AL_EAXREVERB_DECAY_LFRATIO] = _decayLFRatio;
		properties_f[AL_EAXREVERB_REFLECTIONS_GAIN] = _reflectionGain;
		properties_f[AL_EAXREVERB_REFLECTIONS_DELAY] = _reflectionDelay;
		properties_v[AL_EAXREVERB_REFLECTIONS_PAN] = _reflectionPan;
		properties_f[AL_EAXREVERB_LATE_REVERB_GAIN] = _lateReverbGain;
		properties_f[AL_EAXREVERB_LATE_REVERB_DELAY] = _lateReverbDelay;
		properties_v[AL_EAXREVERB_LATE_REVERB_PAN] = _lateReverbPan;
		properties_f[AL_EAXREVERB_ECHO_TIME] = _echoTime;
		properties_f[AL_EAXREVERB_ECHO_DEPTH] = _echoDepth;
		properties_f[AL_EAXREVERB_MODULATION_TIME] = _modTime;
		properties_f[AL_EAXREVERB_MODULATION_DEPTH] = _modDepth;
		properties_f[AL_EAXREVERB_AIR_ABSORPTION_GAINHF] = _airAbsorptionGainHF;
		properties_f[AL_EAXREVERB_HFREFERENCE] = _hfReference;
		properties_f[AL_EAXREVERB_LFREFERENCE] = _lfReference;
		properties_f[AL_EAXREVERB_ROOM_ROLLOFF_FACTOR] = _roomRollOffFactor;

		filter_properties_f[AL_LOWPASS_GAIN] = 1.0f;
		filter_properties_f[AL_LOWPASS_GAINHF] = 1.0f;
	}

	EAXSfxProps() {}

	// Reverb
	std::map<ALuint, ALfloat> properties_f;
	std::map<ALuint, ALint>   properties_i;
	std::map<ALuint, float3>  properties_v;

	// Filter
	std::map<ALuint, ALfloat> filter_properties_f;
};

namespace EFXParamTypes {
	enum {
		FLOAT,
		VECTOR,
		BOOL
	};
}

extern std::map<std::string, EAXSfxProps> eaxPresets;

extern std::map<ALuint, unsigned> alParamType;
extern std::map<std::string, ALuint> nameToALParam;
extern std::map<ALuint, std::string> alParamToName;
extern std::map<std::string, ALuint> nameToALFilterParam;
extern std::map<ALuint, std::string> alFilterParamToName;

#endif // _EFX_PRESETS_H_