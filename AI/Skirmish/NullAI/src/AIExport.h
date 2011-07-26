/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AIEXPORT_H
#define _AIEXPORT_H

// check if the correct defines are set by the build system
#if !defined BUILDING_SKIRMISH_AI
#	error BUILDING_SKIRMISH_AI should be defined when building Skirmish AIs
#endif
#if !defined BUILDING_AI
#	error BUILDING_AI should be defined when building Skirmish AIs
#endif
#if defined BUILDING_AI_INTERFACE
#	error BUILDING_AI_INTERFACE should not be defined when building Skirmish AIs
#endif
#if defined SYNCIFY
#	error SYNCIFY should not be defined when building Skirmish AIs
#endif


#include "ExternalAI/Interface/aidefines.h"
//#include "ExternalAI/Interface/ELevelOfSupport.h"
//struct SSkirmishAICallback;

// for a list of the functions that have to be exported,
// see struct SSkirmishAILibrary in "ExternalAI/Interface/SSkirmishAILibrary.h"

// static AI library methods (optional to implement)
//EXPORT(enum LevelOfSupport) getLevelOfSupportFor(
//		const char* aiShortName, const char* aiVersion,
//		const char* engineVersionString, int engineVersionNumber,
//		const char* aiInterfaceShortName, const char* aiInterfaceVersion);

// instance functions
//EXPORT(int) init(int skirmishAIId,
//		const struct SSkirmishAICallback* callback);
//EXPORT(int) release(int skirmishAIId);
EXPORT(int) handleEvent(int skirmishAIId, int topic, const void* data);

#endif // _AIEXPORT_H
