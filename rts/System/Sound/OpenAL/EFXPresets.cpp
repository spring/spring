/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "EFXPresets.h"
#include "System/StringUtil.h"

spring::unsynced_map<std::string, EAXSfxProps> eaxPresets;

spring::unsynced_map<ALuint, unsigned> alParamType;
spring::unsynced_map<std::string, ALuint> nameToALParam;
spring::unsynced_map<ALuint, std::string> alParamToName;
spring::unsynced_map<std::string, ALuint> nameToALFilterParam;
spring::unsynced_map<ALuint, std::string> alFilterParamToName;

static void InitPresets()
{
	//source: EFX-Util.h from the OpenAL1.1 SDK

	// STANDARD PRESETS
	eaxPresets["generic"] = EAXSfxProps(1, 1, 0.316228, 0.891251, 1, 1.49, 0.83, 1, 0.0500035, 0.007, float3(0, 0, 0.3), 1.25893, 0.011, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 5000, 250, 1, AL_TRUE);
	eaxPresets["paddedcell"] = EAXSfxProps(0.1715, 1, 0.316228, 0.001, 1, 0.17, 0.1, 1, 0.250035, 0.001, float3(0, 0, 0.3), 1.26911, 0.002, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 5000, 250, 1, AL_TRUE);
	eaxPresets["room"] = EAXSfxProps(0.428687, 1, 0.316228, 0.592925, 1, 0.4, 0.83, 1, 0.150314, 0.002, float3(0, 0, 0.3), 1.06292, 0.003, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 5000, 250, 1, AL_TRUE);
	eaxPresets["bathroom"] = EAXSfxProps(0.1715, 1, 0.316228, 0.251189, 1, 1.49, 0.54, 1, 0.653131, 0.007, float3(0, 0, 0.3), 3.27341, 0.011, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 5000, 250, 1, AL_TRUE);
	eaxPresets["livingroom"] = EAXSfxProps(0.976562, 1, 0.316228, 0.001, 1, 0.5, 0.1, 1, 0.205116, 0.003, float3(0, 0, 0.3), 0.280543, 0.004, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 5000, 250, 1, AL_TRUE);
	eaxPresets["stoneroom"] = EAXSfxProps(1, 1, 0.316228, 0.707946, 1, 2.31, 0.64, 1, 0.441062, 0.012, float3(0, 0, 0.3), 1.10027, 0.017, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 5000, 250, 1, AL_TRUE);
	eaxPresets["auditorium"] = EAXSfxProps(1, 1, 0.316228, 0.578096, 1, 4.32, 0.59, 1, 0.403181, 0.02, float3(0, 0, 0.3), 0.716968, 0.03, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 5000, 250, 1, AL_TRUE);
	eaxPresets["concerthall"] = EAXSfxProps(1, 1, 0.316228, 0.562341, 1, 3.92, 0.7, 1, 0.242661, 0.02, float3(0, 0, 0.3), 0.9977, 0.029, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 5000, 250, 1, AL_TRUE);
	eaxPresets["cave"] = EAXSfxProps(1, 1, 0.316228, 1, 1, 2.91, 1.3, 1, 0.500035, 0.015, float3(0, 0, 0.3), 0.706318, 0.022, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 5000, 250, 1, AL_FALSE);
	eaxPresets["arena"] = EAXSfxProps(1, 1, 0.316228, 0.447713, 1, 7.24, 0.33, 1, 0.261216, 0.02, float3(0, 0, 0.3), 1.01859, 0.03, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 5000, 250, 1, AL_TRUE);
	eaxPresets["hangar"] = EAXSfxProps(1, 1, 0.316228, 0.316228, 1, 10.05, 0.23, 1, 0.500035, 0.02, float3(0, 0, 0.3), 1.25603, 0.03, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 5000, 250, 1, AL_TRUE);
	eaxPresets["carpettedhallway"] = EAXSfxProps(0.428687, 1, 0.316228, 0.01, 1, 0.3, 0.1, 1, 0.121479, 0.002, float3(0, 0, 0.3), 0.153109, 0.03, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 5000, 250, 1, AL_TRUE);
	eaxPresets["hallway"] = EAXSfxProps(0.3645, 1, 0.316228, 0.707946, 1, 1.49, 0.59, 1, 0.245754, 0.007, float3(0, 0, 0.3), 1.6615, 0.011, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 5000, 250, 1, AL_TRUE);
	eaxPresets["stonecorridor"] = EAXSfxProps(1, 1, 0.316228, 0.761202, 1, 2.7, 0.79, 1, 0.247172, 0.013, float3(0, 0, 0.3), 1.5758, 0.02, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 5000, 250, 1, AL_TRUE);
	eaxPresets["alley"] = EAXSfxProps(1, 0.3, 0.316228, 0.732825, 1, 1.49, 0.86, 1, 0.250035, 0.007, float3(0, 0, 0.3), 0.995405, 0.011, float3(0, 0, 0.3), 0.125, 0.95, 0.25, 0, 0.99426, 5000, 250, 1, AL_TRUE);
	eaxPresets["forest"] = EAXSfxProps(1, 0.3, 0.316228, 0.0223872, 1, 1.49, 0.54, 1, 0.0524807, 0.162, float3(0, 0, 0.3), 0.768245, 0.088, float3(0, 0, 0.3), 0.125, 1, 0.25, 0, 0.99426, 5000, 250, 1, AL_TRUE);
	eaxPresets["city"] = EAXSfxProps(1, 0.5, 0.316228, 0.398107, 1, 1.49, 0.67, 1, 0.0730298, 0.007, float3(0, 0, 0.3), 0.142725, 0.011, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 5000, 250, 1, AL_TRUE);
	eaxPresets["mountains"] = EAXSfxProps(1, 0.27, 0.316228, 0.0562341, 1, 1.49, 0.21, 1, 0.040738, 0.3, float3(0, 0, 0.3), 0.191867, 0.1, float3(0, 0, 0.3), 0.25, 1, 0.25, 0, 0.99426, 5000, 250, 1, AL_FALSE);
	eaxPresets["quarry"] = EAXSfxProps(1, 1, 0.316228, 0.316228, 1, 1.49, 0.83, 1, 0, 0.061, float3(0, 0, 0.3), 1.77828, 0.025, float3(0, 0, 0.3), 0.125, 0.7, 0.25, 0, 0.99426, 5000, 250, 1, AL_TRUE);
	eaxPresets["plain"] = EAXSfxProps(1, 0.21, 0.316228, 0.1, 1, 1.49, 0.5, 1, 0.058479, 0.179, float3(0, 0, 0.3), 0.108893, 0.1, float3(0, 0, 0.3), 0.25, 1, 0.25, 0, 0.99426, 5000, 250, 1, AL_TRUE);
	eaxPresets["parkinglot"] = EAXSfxProps(1, 1, 0.316228, 1, 1, 1.65, 1.5, 1, 0.208209, 0.008, float3(0, 0, 0.3), 0.265155, 0.012, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 5000, 250, 1, AL_FALSE);
	eaxPresets["sewerpipe"] = EAXSfxProps(0.307063, 0.8, 0.316228, 0.316228, 1, 2.81, 0.14, 1, 1.6387, 0.014, float3(0, 0, 0.3), 3.24713, 0.021, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 5000, 250, 1, AL_TRUE);
	eaxPresets["underwater"] = EAXSfxProps(0.3645, 1, 0.316228, 0.01, 1, 1.49, 0.1, 1, 0.596348, 0.007, float3(0, 0, 0.3), 7.07946, 0.011, float3(0, 0, 0.3), 0.25, 0, 1.18, 0.348, 0.99426, 5000, 250, 1, AL_TRUE);
	eaxPresets["drugged"] = EAXSfxProps(0.428687, 0.5, 0.316228, 1, 1, 8.39, 1.39, 1, 0.875992, 0.002, float3(0, 0, 0.3), 3.10814, 0.03, float3(0, 0, 0.3), 0.25, 0, 0.25, 1, 0.99426, 5000, 250, 1, AL_FALSE);
	eaxPresets["dizzy"] = EAXSfxProps(0.3645, 0.6, 0.316228, 0.630957, 1, 17.23, 0.56, 1, 0.139155, 0.02, float3(0, 0, 0.3), 0.493742, 0.03, float3(0, 0, 0.3), 0.25, 1, 0.81, 0.31, 0.99426, 5000, 250, 1, AL_FALSE);
	eaxPresets["psychotic"] = EAXSfxProps(0.0625, 0.5, 0.316228, 0.840427, 1, 7.56, 0.91, 1, 0.486407, 0.02, float3(0, 0, 0.3), 2.43781, 0.03, float3(0, 0, 0.3), 0.25, 0, 4, 1, 0.99426, 5000, 250, 1, AL_FALSE);

	// CASTLE PRESETS
	eaxPresets["castle_smallroom"] = EAXSfxProps(1, 0.89, 0.316228, 0.398107, 0.1, 1.22, 0.83, 0.31, 0.891251, 0.022, float3(0, 0, 0.3), 1.99526, 0.011, float3(0, 0, 0.3), 0.138, 0.08, 0.25, 0, 0.99426, 5168.6, 139.5, 1, AL_TRUE);
	eaxPresets["castle_shortpassage"] = EAXSfxProps(1, 0.89, 0.316228, 0.316228, 0.1, 2.32, 0.83, 0.31, 0.891251, 0.007, float3(0, 0, 0.3), 1.25893, 0.023, float3(0, 0, 0.3), 0.138, 0.08, 0.25, 0, 0.99426, 5168.6, 139.5, 1, AL_TRUE);
	eaxPresets["castle_mediumroom"] = EAXSfxProps(1, 0.93, 0.316228, 0.281838, 0.1, 2.04, 0.83, 0.46, 0.630957, 0.022, float3(0, 0, 0.3), 1.58489, 0.011, float3(0, 0, 0.3), 0.155, 0.03, 0.25, 0, 0.99426, 5168.6, 139.5, 1, AL_TRUE);
	eaxPresets["castle_longpassage"] = EAXSfxProps(1, 0.89, 0.316228, 0.398107, 0.1, 3.42, 0.83, 0.31, 0.891251, 0.007, float3(0, 0, 0.3), 1.41254, 0.023, float3(0, 0, 0.3), 0.138, 0.08, 0.25, 0, 0.99426, 5168.6, 139.5, 1, AL_TRUE);
	eaxPresets["castle_largeroom"] = EAXSfxProps(1, 0.82, 0.316228, 0.281838, 0.125893, 2.53, 0.83, 0.5, 0.446684, 0.034, float3(0, 0, 0.3), 1.25893, 0.016, float3(0, 0, 0.3), 0.185, 0.07, 0.25, 0, 0.99426, 5168.6, 139.5, 1, AL_TRUE);
	eaxPresets["castle_hall"] = EAXSfxProps(1, 0.81, 0.316228, 0.281838, 0.177828, 3.14, 0.79, 0.62, 0.177828, 0.056, float3(0, 0, 0.3), 1.12202, 0.024, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 5168.6, 139.5, 1, AL_TRUE);
	eaxPresets["castle_cupboard"] = EAXSfxProps(1, 0.89, 0.316228, 0.281838, 0.1, 0.67, 0.87, 0.31, 1.41254, 0.01, float3(0, 0, 0.3), 3.54813, 0.007, float3(0, 0, 0.3), 0.138, 0.08, 0.25, 0, 0.99426, 5168.6, 139.5, 1, AL_TRUE);
	eaxPresets["castle_courtyard"] = EAXSfxProps(1, 0.42, 0.316228, 0.446684, 0.199526, 2.13, 0.61, 0.23, 0.223872, 0.16, float3(0, 0, 0.3), 0.707946, 0.036, float3(0, 0, 0.3), 0.25, 0.37, 0.25, 0, 0.99426, 5000, 250, 1, AL_FALSE);
	eaxPresets["castle_alcove"] = EAXSfxProps(1, 0.89, 0.316228, 0.501187, 0.1, 1.64, 0.87, 0.31, 1, 0.007, float3(0, 0, 0.3), 1.41254, 0.034, float3(0, 0, 0.3), 0.138, 0.08, 0.25, 0, 0.99426, 5168.6, 139.5, 1, AL_TRUE);

	// FACTORY PRESETS
	eaxPresets["factory_alcove"] = EAXSfxProps(0.3645, 0.59, 0.251189, 0.794328, 0.501187, 3.14, 0.65, 1.31, 1.41254, 0.01, float3(0, 0, 0.3), 1, 0.038, float3(0, 0, 0.3), 0.114, 0.1, 0.25, 0, 0.99426, 3762.6, 362.5, 1, AL_TRUE);
	eaxPresets["factory_shortpassage"] = EAXSfxProps(0.3645, 0.64, 0.251189, 0.794328, 0.501187, 2.53, 0.65, 1.31, 1, 0.01, float3(0, 0, 0.3), 1.25893, 0.038, float3(0, 0, 0.3), 0.135, 0.23, 0.25, 0, 0.99426, 3762.6, 362.5, 1, AL_TRUE);
	eaxPresets["factory_mediumroom"] = EAXSfxProps(0.428687, 0.82, 0.251189, 0.794328, 0.501187, 2.76, 0.65, 1.31, 0.281838, 0.022, float3(0, 0, 0.3), 1.41254, 0.023, float3(0, 0, 0.3), 0.174, 0.07, 0.25, 0, 0.99426, 3762.6, 362.5, 1, AL_TRUE);
	eaxPresets["factory_longpassage"] = EAXSfxProps(0.3645, 0.64, 0.251189, 0.794328, 0.501187, 4.06, 0.65, 1.31, 1, 0.02, float3(0, 0, 0.3), 1.25893, 0.037, float3(0, 0, 0.3), 0.135, 0.23, 0.25, 0, 0.99426, 3762.6, 362.5, 1, AL_TRUE);
	eaxPresets["factory_largeroom"] = EAXSfxProps(0.428687, 0.75, 0.251189, 0.707946, 0.630957, 4.24, 0.51, 1.31, 0.177828, 0.039, float3(0, 0, 0.3), 1.12202, 0.023, float3(0, 0, 0.3), 0.231, 0.07, 0.25, 0, 0.99426, 3762.6, 362.5, 1, AL_TRUE);
	eaxPresets["factory_hall"] = EAXSfxProps(0.428687, 0.75, 0.316228, 0.707946, 0.630957, 7.43, 0.51, 1.31, 0.0630957, 0.073, float3(0, 0, 0.3), 0.891251, 0.027, float3(0, 0, 0.3), 0.25, 0.07, 0.25, 0, 0.99426, 3762.6, 362.5, 1, AL_TRUE);
	eaxPresets["factory_cupboard"] = EAXSfxProps(0.307063, 0.63, 0.251189, 0.794328, 0.501187, 0.49, 0.65, 1.31, 1.25893, 0.01, float3(0, 0, 0.3), 1.99526, 0.032, float3(0, 0, 0.3), 0.107, 0.07, 0.25, 0, 0.99426, 3762.6, 362.5, 1, AL_TRUE);
	eaxPresets["factory_courtyard"] = EAXSfxProps(0.307063, 0.57, 0.316228, 0.316228, 0.630957, 2.32, 0.29, 0.56, 0.223872, 0.14, float3(0, 0, 0.3), 0.398107, 0.039, float3(0, 0, 0.3), 0.25, 0.29, 0.25, 0, 0.99426, 3762.6, 362.5, 1, AL_TRUE);
	eaxPresets["factory_smallroom"] = EAXSfxProps(0.3645, 0.82, 0.316228, 0.794328, 0.501187, 1.72, 0.65, 1.31, 0.707946, 0.01, float3(0, 0, 0.3), 1.77828, 0.024, float3(0, 0, 0.3), 0.119, 0.07, 0.25, 0, 0.99426, 3762.6, 362.5, 1, AL_TRUE);

	// ICE PALACE PRESETS
	eaxPresets["icepalace_alcove"] = EAXSfxProps(1, 0.84, 0.316228, 0.562341, 0.281838, 2.76, 1.46, 0.28, 1.12202, 0.01, float3(0, 0, 0.3), 0.891251, 0.03, float3(0, 0, 0.3), 0.161, 0.09, 0.25, 0, 0.99426, 12428.5, 99.6, 1, AL_TRUE);
	eaxPresets["icepalace_shortpassage"] = EAXSfxProps(1, 0.75, 0.316228, 0.562341, 0.281838, 1.79, 1.46, 0.28, 0.501187, 0.01, float3(0, 0, 0.3), 1.12202, 0.019, float3(0, 0, 0.3), 0.177, 0.09, 0.25, 0, 0.99426, 12428.5, 99.6, 1, AL_TRUE);
	eaxPresets["icepalace_mediumroom"] = EAXSfxProps(1, 0.87, 0.316228, 0.562341, 0.446684, 2.22, 1.53, 0.32, 0.398107, 0.039, float3(0, 0, 0.3), 1.12202, 0.027, float3(0, 0, 0.3), 0.186, 0.12, 0.25, 0, 0.99426, 12428.5, 99.6, 1, AL_TRUE);
	eaxPresets["icepalace_longpassage"] = EAXSfxProps(1, 0.77, 0.316228, 0.562341, 0.398107, 3.01, 1.46, 0.28, 0.794328, 0.012, float3(0, 0, 0.3), 1.25893, 0.025, float3(0, 0, 0.3), 0.186, 0.04, 0.25, 0, 0.99426, 12428.5, 99.6, 1, AL_TRUE);
	eaxPresets["icepalace_largeroom"] = EAXSfxProps(1, 0.81, 0.316228, 0.562341, 0.446684, 3.14, 1.53, 0.32, 0.251189, 0.039, float3(0, 0, 0.3), 1, 0.027, float3(0, 0, 0.3), 0.214, 0.11, 0.25, 0, 0.99426, 12428.5, 99.6, 1, AL_TRUE);
	eaxPresets["icepalace_hall"] = EAXSfxProps(1, 0.76, 0.316228, 0.446684, 0.562341, 5.49, 1.53, 0.38, 0.112202, 0.054, float3(0, 0, 0.3), 0.630957, 0.052, float3(0, 0, 0.3), 0.226, 0.11, 0.25, 0, 0.99426, 12428.5, 99.6, 1, AL_TRUE);
	eaxPresets["icepalace_cupboard"] = EAXSfxProps(1, 0.83, 0.316228, 0.501187, 0.223872, 0.76, 1.53, 0.26, 1.12202, 0.012, float3(0, 0, 0.3), 1.99526, 0.016, float3(0, 0, 0.3), 0.143, 0.08, 0.25, 0, 0.99426, 12428.5, 99.6, 1, AL_TRUE);
	eaxPresets["icepalace_courtyard"] = EAXSfxProps(1, 0.59, 0.316228, 0.281838, 0.316228, 2.04, 1.2, 0.38, 0.316228, 0.173, float3(0, 0, 0.3), 0.316228, 0.043, float3(0, 0, 0.3), 0.235, 0.48, 0.25, 0, 0.99426, 12428.5, 99.6, 1, AL_TRUE);
	eaxPresets["icepalace_smallroom"] = EAXSfxProps(1, 0.84, 0.316228, 0.562341, 0.281838, 1.51, 1.53, 0.27, 0.891251, 0.01, float3(0, 0, 0.3), 1.41254, 0.011, float3(0, 0, 0.3), 0.164, 0.14, 0.25, 0, 0.99426, 12428.5, 99.6, 1, AL_TRUE);

	// SPACE STATION PRESETS
	eaxPresets["spacestation_alcove"] = EAXSfxProps(0.210938, 0.78, 0.316228, 0.707946, 0.891251, 1.16, 0.81, 0.55, 1.41254, 0.007, float3(0, 0, 0.3), 1, 0.018, float3(0, 0, 0.3), 0.192, 0.21, 0.25, 0, 0.99426, 3316.1, 458.2, 1, AL_TRUE);
	eaxPresets["spacestation_mediumroom"] = EAXSfxProps(0.210938, 0.75, 0.316228, 0.630957, 0.891251, 3.01, 0.5, 0.55, 0.398107, 0.034, float3(0, 0, 0.3), 1.12202, 0.035, float3(0, 0, 0.3), 0.209, 0.31, 0.25, 0, 0.99426, 3316.1, 458.2, 1, AL_TRUE);
	eaxPresets["spacestation_shortpassage"] = EAXSfxProps(0.210938, 0.87, 0.316228, 0.630957, 0.891251, 3.57, 0.5, 0.55, 1, 0.012, float3(0, 0, 0.3), 1.12202, 0.016, float3(0, 0, 0.3), 0.172, 0.2, 0.25, 0, 0.99426, 3316.1, 458.2, 1, AL_TRUE);
	eaxPresets["spacestation_longpassage"] = EAXSfxProps(0.428687, 0.82, 0.316228, 0.630957, 0.891251, 4.62, 0.62, 0.55, 1, 0.012, float3(0, 0, 0.3), 1.25893, 0.031, float3(0, 0, 0.3), 0.25, 0.23, 0.25, 0, 0.99426, 3316.1, 458.2, 1, AL_TRUE);
	eaxPresets["spacestation_largeroom"] = EAXSfxProps(0.3645, 0.81, 0.316228, 0.630957, 0.891251, 3.89, 0.38, 0.61, 0.316228, 0.056, float3(0, 0, 0.3), 0.891251, 0.035, float3(0, 0, 0.3), 0.233, 0.28, 0.25, 0, 0.99426, 3316.1, 458.2, 1, AL_TRUE);
	eaxPresets["spacestation_hall"] = EAXSfxProps(0.428687, 0.87, 0.316228, 0.630957, 0.891251, 7.11, 0.38, 0.61, 0.177828, 0.1, float3(0, 0, 0.3), 0.630957, 0.047, float3(0, 0, 0.3), 0.25, 0.25, 0.25, 0, 0.99426, 3316.1, 458.2, 1, AL_TRUE);
	eaxPresets["spacestation_cupboard"] = EAXSfxProps(0.1715, 0.56, 0.316228, 0.707946, 0.891251, 0.79, 0.81, 0.55, 1.41254, 0.007, float3(0, 0, 0.3), 1.77828, 0.018, float3(0, 0, 0.3), 0.181, 0.31, 0.25, 0, 0.99426, 3316.1, 458.2, 1, AL_TRUE);
	eaxPresets["spacestation_smallroom"] = EAXSfxProps(0.210938, 0.7, 0.316228, 0.707946, 0.891251, 1.72, 0.82, 0.55, 0.794328, 0.007, float3(0, 0, 0.3), 1.41254, 0.013, float3(0, 0, 0.3), 0.188, 0.26, 0.25, 0, 0.99426, 3316.1, 458.2, 1, AL_TRUE);

	// WOODEN GALLEON PRESETS
	eaxPresets["wooden_alcove"] = EAXSfxProps(1, 1, 0.316228, 0.125893, 0.316228, 1.22, 0.62, 0.91, 1.12202, 0.012, float3(0, 0, 0.3), 0.707946, 0.024, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 4705, 99.6, 1, AL_TRUE);
	eaxPresets["wooden_shortpassage"] = EAXSfxProps(1, 1, 0.316228, 0.125893, 0.316228, 1.75, 0.5, 0.87, 0.891251, 0.012, float3(0, 0, 0.3), 0.630957, 0.024, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 4705, 99.6, 1, AL_TRUE);
	eaxPresets["wooden_mediumroom"] = EAXSfxProps(1, 1, 0.316228, 0.1, 0.281838, 1.47, 0.42, 0.82, 0.891251, 0.049, float3(0, 0, 0.3), 0.891251, 0.029, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 4705, 99.6, 1, AL_TRUE);
	eaxPresets["wooden_longpassage"] = EAXSfxProps(1, 1, 0.316228, 0.1, 0.316228, 1.99, 0.4, 0.79, 1, 0.02, float3(0, 0, 0.3), 0.446684, 0.036, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 4705, 99.6, 1, AL_TRUE);
	eaxPresets["wooden_largeroom"] = EAXSfxProps(1, 1, 0.316228, 0.0891251, 0.281838, 2.65, 0.33, 0.82, 0.891251, 0.066, float3(0, 0, 0.3), 0.794328, 0.049, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 4705, 99.6, 1, AL_TRUE);
	eaxPresets["wooden_hall"] = EAXSfxProps(1, 1, 0.316228, 0.0794328, 0.281838, 3.45, 0.3, 0.82, 0.891251, 0.088, float3(0, 0, 0.3), 0.794328, 0.063, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 4705, 99.6, 1, AL_TRUE);
	eaxPresets["wooden_cupboard"] = EAXSfxProps(1, 1, 0.316228, 0.141254, 0.316228, 0.56, 0.46, 0.91, 1.12202, 0.012, float3(0, 0, 0.3), 1.12202, 0.028, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 4705, 99.6, 1, AL_TRUE);
	eaxPresets["wooden_smallroom"] = EAXSfxProps(1, 1, 0.316228, 0.112202, 0.316228, 0.79, 0.32, 0.87, 1, 0.032, float3(0, 0, 0.3), 0.891251, 0.029, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 4705, 99.6, 1, AL_TRUE);
	eaxPresets["wooden_courtyard"] = EAXSfxProps(1, 0.65, 0.316228, 0.0794328, 0.316228, 1.79, 0.35, 0.79, 0.562341, 0.123, float3(0, 0, 0.3), 0.1, 0.032, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 4705, 99.6, 1, AL_TRUE);

	// SPORTS PRESETS
	eaxPresets["sport_emptystadium"] = EAXSfxProps(1, 1, 0.316228, 0.446684, 0.794328, 6.26, 0.51, 1.1, 0.0630957, 0.183, float3(0, 0, 0.3), 0.398107, 0.038, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 5000, 250, 1, AL_TRUE);
	eaxPresets["sport_squashcourt"] = EAXSfxProps(1, 0.75, 0.316228, 0.316228, 0.794328, 2.22, 0.91, 1.16, 0.446684, 0.007, float3(0, 0, 0.3), 0.794328, 0.011, float3(0, 0, 0.3), 0.126, 0.19, 0.25, 0, 0.99426, 7176.9, 211.2, 1, AL_TRUE);
	eaxPresets["sport_smallswimmingpool"] = EAXSfxProps(1, 0.7, 0.316228, 0.794328, 0.891251, 2.76, 1.25, 1.14, 0.630957, 0.02, float3(0, 0, 0.3), 0.794328, 0.03, float3(0, 0, 0.3), 0.179, 0.15, 0.895, 0.19, 0.99426, 5000, 250, 1, AL_FALSE);
	eaxPresets["sport_largeswimmingpool"] = EAXSfxProps(1, 0.82, 0.316228, 0.794328, 1, 5.49, 1.31, 1.14, 0.446684, 0.039, float3(0, 0, 0.3), 0.501187, 0.049, float3(0, 0, 0.3), 0.222, 0.55, 1.159, 0.21, 0.99426, 5000, 250, 1, AL_FALSE);
	eaxPresets["sport_gymnasium"] = EAXSfxProps(1, 0.81, 0.316228, 0.446684, 0.891251, 3.14, 1.06, 1.35, 0.398107, 0.029, float3(0, 0, 0.3), 0.562341, 0.045, float3(0, 0, 0.3), 0.146, 0.14, 0.25, 0, 0.99426, 7176.9, 211.2, 1, AL_TRUE);
	eaxPresets["sport_fullstadium"] = EAXSfxProps(1, 1, 0.316228, 0.0707946, 0.794328, 5.25, 0.17, 0.8, 0.1, 0.188, float3(0, 0, 0.3), 0.281838, 0.038, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 5000, 250, 1, AL_TRUE);
	eaxPresets["sport_stadiumtannoy"] = EAXSfxProps(1, 0.78, 0.316228, 0.562341, 0.501187, 2.53, 0.88, 0.68, 0.281838, 0.23, float3(0, 0, 0.3), 0.501187, 0.063, float3(0, 0, 0.3), 0.25, 0.2, 0.25, 0, 0.99426, 5000, 250, 1, AL_TRUE);

	// PREFAB PRESETS
	eaxPresets["prefab_workshop"] = EAXSfxProps(0.428687, 1, 0.316228, 0.141254, 0.398107, 0.76, 1, 1, 1, 0.012, float3(0, 0, 0.3), 1.12202, 0.012, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 5000, 250, 1, AL_FALSE);
	eaxPresets["prefab_schoolroom"] = EAXSfxProps(0.402178, 0.69, 0.316228, 0.630957, 0.501187, 0.98, 0.45, 0.18, 1.41254, 0.017, float3(0, 0, 0.3), 1.41254, 0.015, float3(0, 0, 0.3), 0.095, 0.14, 0.25, 0, 0.99426, 7176.9, 211.2, 1, AL_TRUE);
	eaxPresets["prefab_practiseroom"] = EAXSfxProps(0.402178, 0.87, 0.316228, 0.398107, 0.501187, 1.12, 0.56, 0.18, 1.25893, 0.01, float3(0, 0, 0.3), 1.41254, 0.011, float3(0, 0, 0.3), 0.095, 0.14, 0.25, 0, 0.99426, 7176.9, 211.2, 1, AL_TRUE);
	eaxPresets["prefab_outhouse"] = EAXSfxProps(1, 0.82, 0.316228, 0.112202, 0.158489, 1.38, 0.38, 0.35, 0.891251, 0.024, float3(0, 0, 0.3), 0.630957, 0.044, float3(0, 0, 0.3), 0.121, 0.17, 0.25, 0, 0.99426, 2854.4, 107.5, 1, AL_FALSE);
	eaxPresets["prefab_caravan"] = EAXSfxProps(1, 1, 0.316228, 0.0891251, 0.125893, 0.43, 1.5, 1, 1, 0.012, float3(0, 0, 0.3), 1.99526, 0.012, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 5000, 250, 1, AL_FALSE);

	// DOME AND PIPE PRESETS
	eaxPresets["dome_tomb"] = EAXSfxProps(1, 0.79, 0.316228, 0.354813, 0.223872, 4.18, 0.21, 0.1, 0.386812, 0.03, float3(0, 0, 0.3), 1.6788, 0.022, float3(0, 0, 0.3), 0.177, 0.19, 0.25, 0, 0.99426, 2854.4, 20, 1, AL_FALSE);
	eaxPresets["pipe_small"] = EAXSfxProps(1, 1, 0.316228, 0.354813, 0.223872, 5.04, 0.1, 0.1, 0.501187, 0.032, float3(0, 0, 0.3), 2.51189, 0.015, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 2854.4, 20, 1, AL_TRUE);
	eaxPresets["dome_saintpauls"] = EAXSfxProps(1, 0.87, 0.316228, 0.354813, 0.223872, 10.48, 0.19, 0.1, 0.177828, 0.09, float3(0, 0, 0.3), 1.25893, 0.042, float3(0, 0, 0.3), 0.25, 0.12, 0.25, 0, 0.99426, 2854.4, 20, 1, AL_TRUE);
	eaxPresets["pipe_longthin"] = EAXSfxProps(0.256, 0.91, 0.316228, 0.446684, 0.281838, 9.21, 0.18, 0.1, 0.707946, 0.01, float3(0, 0, 0.3), 0.707946, 0.022, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 2854.4, 20, 1, AL_FALSE);
	eaxPresets["pipe_large"] = EAXSfxProps(1, 1, 0.316228, 0.354813, 0.223872, 8.45, 0.1, 0.1, 0.398107, 0.046, float3(0, 0, 0.3), 1.58489, 0.032, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 2854.4, 20, 1, AL_TRUE);
	eaxPresets["pipe_resonant"] = EAXSfxProps(0.137312, 0.91, 0.316228, 0.446684, 0.281838, 6.81, 0.18, 0.1, 0.707946, 0.01, float3(0, 0, 0.3), 1, 0.022, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 2854.4, 20, 1, AL_FALSE);

	// OUTDOORS PRESETS
	eaxPresets["outdoors_backyard"] = EAXSfxProps(1, 0.45, 0.316228, 0.251189, 0.501187, 1.12, 0.34, 0.46, 0.446684, 0.069, float3(0, 0, 0.3), 0.707946, 0.023, float3(0, 0, 0.3), 0.218, 0.34, 0.25, 0, 0.99426, 4399.1, 242.9, 1, AL_FALSE);
	eaxPresets["outdoors_rollingplains"] = EAXSfxProps(1, 0, 0.316228, 0.0112202, 0.630957, 2.13, 0.21, 0.46, 0.177828, 0.3, float3(0, 0, 0.3), 0.446684, 0.019, float3(0, 0, 0.3), 0.25, 1, 0.25, 0, 0.99426, 4399.1, 242.9, 1, AL_FALSE);
	eaxPresets["outdoors_deepcanyon"] = EAXSfxProps(1, 0.74, 0.316228, 0.177828, 0.630957, 3.89, 0.21, 0.46, 0.316228, 0.223, float3(0, 0, 0.3), 0.354813, 0.019, float3(0, 0, 0.3), 0.25, 1, 0.25, 0, 0.99426, 4399.1, 242.9, 1, AL_FALSE);
	eaxPresets["outdoors_creek"] = EAXSfxProps(1, 0.35, 0.316228, 0.177828, 0.501187, 2.13, 0.21, 0.46, 0.398107, 0.115, float3(0, 0, 0.3), 0.199526, 0.031, float3(0, 0, 0.3), 0.218, 0.34, 0.25, 0, 0.99426, 4399.1, 242.9, 1, AL_FALSE);
	eaxPresets["outdoors_valley"] = EAXSfxProps(1, 0.28, 0.316228, 0.0281838, 0.158489, 2.88, 0.26, 0.35, 0.141254, 0.263, float3(0, 0, 0.3), 0.398107, 0.1, float3(0, 0, 0.3), 0.25, 0.34, 0.25, 0, 0.99426, 2854.4, 107.5, 1, AL_FALSE);

	// MOOD PRESETS
	eaxPresets["mood_heaven"] = EAXSfxProps(1, 0.94, 0.316228, 0.794328, 0.446684, 5.04, 1.12, 0.56, 0.242661, 0.02, float3(0, 0, 0.3), 1.25893, 0.029, float3(0, 0, 0.3), 0.25, 0.08, 2.742, 0.05, 0.9977, 5000, 250, 1, AL_TRUE);
	eaxPresets["mood_hell"] = EAXSfxProps(1, 0.57, 0.316228, 0.354813, 0.446684, 3.57, 0.49, 2, 0, 0.02, float3(0, 0, 0.3), 1.41254, 0.03, float3(0, 0, 0.3), 0.11, 0.04, 2.109, 0.52, 0.99426, 5000, 139.5, 1, AL_FALSE);
	eaxPresets["mood_memory"] = EAXSfxProps(1, 0.85, 0.316228, 0.630957, 0.354813, 4.06, 0.82, 0.56, 0.0398107, 0, float3(0, 0, 0.3), 1.12202, 0, float3(0, 0, 0.3), 0.25, 0, 0.474, 0.45, 0.988553, 5000, 250, 1, AL_FALSE);

	// DRIVING SIMULATION PRESETS
	eaxPresets["driving_commentator"] = EAXSfxProps(1, 0, 0.316228, 0.562341, 0.501187, 2.42, 0.88, 0.68, 0.199526, 0.093, float3(0, 0, 0.3), 0.251189, 0.017, float3(0, 0, 0.3), 0.25, 1, 0.25, 0, 0.988553, 5000, 250, 1, AL_TRUE);
	eaxPresets["driving_pitgarage"] = EAXSfxProps(0.428687, 0.59, 0.316228, 0.707946, 0.562341, 1.72, 0.93, 0.87, 0.562341, 0, float3(0, 0, 0.3), 1.25893, 0.016, float3(0, 0, 0.3), 0.25, 0.11, 0.25, 0, 0.99426, 5000, 250, 1, AL_FALSE);
	eaxPresets["driving_incar_racer"] = EAXSfxProps(0.0831875, 0.8, 0.316228, 1, 0.794328, 0.17, 2, 0.41, 1.77828, 0.007, float3(0, 0, 0.3), 0.707946, 0.015, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 10268.2, 251, 1, AL_TRUE);
	eaxPresets["driving_incar_sports"] = EAXSfxProps(0.0831875, 0.8, 0.316228, 0.630957, 1, 0.17, 0.75, 0.41, 1, 0.01, float3(0, 0, 0.3), 0.562341, 0, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 10268.2, 251, 1, AL_TRUE);
	eaxPresets["driving_incar_luxury"] = EAXSfxProps(0.256, 1, 0.316228, 0.1, 0.501187, 0.13, 0.41, 0.46, 0.794328, 0.01, float3(0, 0, 0.3), 1.58489, 0.01, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 10268.2, 251, 1, AL_TRUE);
	eaxPresets["driving_fullgrandstand"] = EAXSfxProps(1, 1, 0.316228, 0.281838, 0.630957, 3.01, 1.37, 1.28, 0.354813, 0.09, float3(0, 0, 0.3), 0.177828, 0.049, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 10420.2, 250, 1, AL_FALSE);
	eaxPresets["driving_emptygrandstand"] = EAXSfxProps(1, 1, 0.316228, 1, 0.794328, 4.62, 1.75, 1.4, 0.208209, 0.09, float3(0, 0, 0.3), 0.251189, 0.049, float3(0, 0, 0.3), 0.25, 0, 0.25, 0, 0.99426, 10420.2, 250, 1, AL_FALSE);
	eaxPresets["driving_tunnel"] = EAXSfxProps(1, 0.81, 0.316228, 0.398107, 0.891251, 3.42, 0.94, 1.31, 0.707946, 0.051, float3(0, 0, 0.3), 0.707946, 0.047, float3(0, 0, 0.3), 0.214, 0.05, 0.25, 0, 0.99426, 5000, 155.3, 1, AL_TRUE);

	// CITY PRESETS
	eaxPresets["city_streets"] = EAXSfxProps(1, 0.78, 0.316228, 0.707946, 0.891251, 1.79, 1.12, 0.91, 0.281838, 0.046, float3(0, 0, 0.3), 0.199526, 0.028, float3(0, 0, 0.3), 0.25, 0.2, 0.25, 0, 0.99426, 5000, 250, 1, AL_TRUE);
	eaxPresets["city_subway"] = EAXSfxProps(1, 0.74, 0.316228, 0.707946, 0.891251, 3.01, 1.23, 0.91, 0.707946, 0.046, float3(0, 0, 0.3), 1.25893, 0.028, float3(0, 0, 0.3), 0.125, 0.21, 0.25, 0, 0.99426, 5000, 250, 1, AL_TRUE);
	eaxPresets["city_museum"] = EAXSfxProps(1, 0.82, 0.316228, 0.177828, 0.177828, 3.28, 1.4, 0.57, 0.251189, 0.039, float3(0, 0, 0.3), 0.891251, 0.034, float3(0, 0, 0.3), 0.13, 0.17, 0.25, 0, 0.99426, 2854.4, 107.5, 1, AL_FALSE);
	eaxPresets["city_library"] = EAXSfxProps(1, 0.82, 0.316228, 0.281838, 0.0891251, 2.76, 0.89, 0.41, 0.354813, 0.029, float3(0, 0, 0.3), 0.891251, 0.02, float3(0, 0, 0.3), 0.13, 0.17, 0.25, 0, 0.99426, 2854.4, 107.5, 1, AL_FALSE);
	eaxPresets["city_underpass"] = EAXSfxProps(1, 0.82, 0.316228, 0.446684, 0.891251, 3.57, 1.12, 0.91, 0.398107, 0.059, float3(0, 0, 0.3), 0.891251, 0.037, float3(0, 0, 0.3), 0.25, 0.14, 0.25, 0, 0.991973, 5000, 250, 1, AL_TRUE);
	eaxPresets["city_abandoned"] = EAXSfxProps(1, 0.69, 0.316228, 0.794328, 0.891251, 3.28, 1.17, 0.91, 0.446684, 0.044, float3(0, 0, 0.3), 0.281838, 0.024, float3(0, 0, 0.3), 0.25, 0.2, 0.25, 0, 0.996552, 5000, 250, 1, AL_TRUE);

	// MISC ROOMS
	eaxPresets["dustyroom"] = EAXSfxProps(0.3645, 0.56, 0.316228, 0.794328, 0.707946, 1.79, 0.38, 0.21, 0.501187, 0.002, float3(0, 0, 0.3), 1.25893, 0.006, float3(0, 0, 0.3), 0.202, 0.05, 0.25, 0, 0.988553, 13046, 163.3, 1, AL_TRUE);
	eaxPresets["chapel"] = EAXSfxProps(1, 0.84, 0.316228, 0.562341, 1, 4.62, 0.64, 1.23, 0.446684, 0.032, float3(0, 0, 0.3), 0.794328, 0.049, float3(0, 0, 0.3), 0.25, 0, 0.25, 0.11, 0.99426, 5000, 250, 1, AL_TRUE);
	eaxPresets["smallwaterroom"] = EAXSfxProps(1, 0.7, 0.316228, 0.447713, 1, 1.51, 1.25, 1.14, 0.891251, 0.02, float3(0, 0, 0.3), 1.41254, 0.03, float3(0, 0, 0.3), 0.179, 0.15, 0.895, 0.19, 0.991973, 5000, 250, 1, AL_FALSE);
}

static void InitConversionTables()
{
	alParamType[AL_EAXREVERB_DENSITY]               = EFXParamTypes::FLOAT;
	alParamType[AL_EAXREVERB_DIFFUSION]             = EFXParamTypes::FLOAT;
	alParamType[AL_EAXREVERB_GAIN]                  = EFXParamTypes::FLOAT;
	alParamType[AL_EAXREVERB_GAINHF]                = EFXParamTypes::FLOAT;
	alParamType[AL_EAXREVERB_GAINLF]                = EFXParamTypes::FLOAT;
	alParamType[AL_EAXREVERB_DECAY_TIME]            = EFXParamTypes::FLOAT;
	alParamType[AL_EAXREVERB_DECAY_HFLIMIT]         = EFXParamTypes::BOOL;
	alParamType[AL_EAXREVERB_DECAY_HFRATIO]         = EFXParamTypes::FLOAT;
	alParamType[AL_EAXREVERB_DECAY_LFRATIO]         = EFXParamTypes::FLOAT;
	alParamType[AL_EAXREVERB_REFLECTIONS_GAIN]      = EFXParamTypes::FLOAT;
	alParamType[AL_EAXREVERB_REFLECTIONS_DELAY]     = EFXParamTypes::FLOAT;
	alParamType[AL_EAXREVERB_REFLECTIONS_PAN]       = EFXParamTypes::VECTOR;
	alParamType[AL_EAXREVERB_LATE_REVERB_GAIN]      = EFXParamTypes::FLOAT;
	alParamType[AL_EAXREVERB_LATE_REVERB_DELAY]     = EFXParamTypes::FLOAT;
	alParamType[AL_EAXREVERB_LATE_REVERB_PAN]       = EFXParamTypes::VECTOR;
	alParamType[AL_EAXREVERB_ECHO_TIME]             = EFXParamTypes::FLOAT;
	alParamType[AL_EAXREVERB_ECHO_DEPTH]            = EFXParamTypes::FLOAT;
	alParamType[AL_EAXREVERB_MODULATION_TIME]       = EFXParamTypes::FLOAT;
	alParamType[AL_EAXREVERB_MODULATION_DEPTH]      = EFXParamTypes::FLOAT;
	alParamType[AL_EAXREVERB_AIR_ABSORPTION_GAINHF] = EFXParamTypes::FLOAT;
	alParamType[AL_EAXREVERB_HFREFERENCE]           = EFXParamTypes::FLOAT;
	alParamType[AL_EAXREVERB_LFREFERENCE]           = EFXParamTypes::FLOAT;
	alParamType[AL_EAXREVERB_ROOM_ROLLOFF_FACTOR]   = EFXParamTypes::FLOAT;
	alParamType[AL_LOWPASS_GAIN]                    = EFXParamTypes::FLOAT;
	alParamType[AL_LOWPASS_GAINHF]                  = EFXParamTypes::FLOAT;


	alParamToName[AL_EAXREVERB_DENSITY]               = "density";
	alParamToName[AL_EAXREVERB_DIFFUSION]             = "diffusion";
	alParamToName[AL_EAXREVERB_GAIN]                  = "gain";
	alParamToName[AL_EAXREVERB_GAINHF]                = "gainhf";
	alParamToName[AL_EAXREVERB_GAINLF]                = "gainlf";
	alParamToName[AL_EAXREVERB_DECAY_TIME]            = "decaytime";
	alParamToName[AL_EAXREVERB_DECAY_HFLIMIT]         = "decayhflimit";
	alParamToName[AL_EAXREVERB_DECAY_HFRATIO]         = "decayhfratio";
	alParamToName[AL_EAXREVERB_DECAY_LFRATIO]         = "decaylfratio";
	alParamToName[AL_EAXREVERB_REFLECTIONS_GAIN]      = "reflectionsgain";
	alParamToName[AL_EAXREVERB_REFLECTIONS_DELAY]     = "reflectionsdelay";
	alParamToName[AL_EAXREVERB_REFLECTIONS_PAN]       = "reflectionspan";
	alParamToName[AL_EAXREVERB_LATE_REVERB_GAIN]      = "latereverbgain";
	alParamToName[AL_EAXREVERB_LATE_REVERB_DELAY]     = "latereverbdelay";
	alParamToName[AL_EAXREVERB_LATE_REVERB_PAN]       = "latereverbpan";
	alParamToName[AL_EAXREVERB_ECHO_TIME]             = "echotime";
	alParamToName[AL_EAXREVERB_ECHO_DEPTH]            = "echodepth";
	alParamToName[AL_EAXREVERB_MODULATION_TIME]       = "modtime";
	alParamToName[AL_EAXREVERB_MODULATION_DEPTH]      = "moddepth";
	alParamToName[AL_EAXREVERB_AIR_ABSORPTION_GAINHF] = "airabsorptiongainhf";
	alParamToName[AL_EAXREVERB_HFREFERENCE]           = "hfreference";
	alParamToName[AL_EAXREVERB_LFREFERENCE]           = "lfreference";
	alParamToName[AL_EAXREVERB_ROOM_ROLLOFF_FACTOR]   = "roomrollofffactor";

	alFilterParamToName[AL_LOWPASS_GAIN]   = "gainlf";
	alFilterParamToName[AL_LOWPASS_GAINHF] = "gainhf";

	for (auto it = alParamToName.begin(); it != alParamToName.end(); ++it)
		nameToALParam[it->second] = it->first;

	for (auto it = alFilterParamToName.begin(); it != alFilterParamToName.end(); ++it)
		nameToALFilterParam[it->second] = it->first;
}

DO_ONCE(InitPresets)
DO_ONCE(InitConversionTables)

