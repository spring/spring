/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _INTERFACE_EXPORT_H
#define _INTERFACE_EXPORT_H

// check if the correct defines are set by the build system
#if !defined BUILDING_AI_INTERFACE
#	error BUILDING_AI_INTERFACE should be defined when building AI Interfaces
#endif
#if !defined BUILDING_AI
#	error BUILDING_AI should be defined when building AI Interfaces
#endif
#if defined BUILDING_SKIRMISH_AI
#	error BUILDING_SKIRMISH_AI should not be defined when building AI Interfaces
#endif
#if defined SYNCIFY
#	error SYNCIFY should not be defined when building AI Interfaces
#endif


#include "ExternalAI/Interface/aidefines.h"

#ifdef __cplusplus
extern "C" {
#endif

//#include "ExternalAI/Interface/ELevelOfSupport.h"

struct SSkirmishAILibrary;
struct SAIInterfaceCallback;

// for a list of the functions that have to be exported,
// see struct SAIInterfaceLibrary in:
// "rts/ExternalAI/Interface/SAIInterfaceLibrary.h"


// static AI interface library functions

EXPORT(int) initStatic(int interfaceId, const SAIInterfaceCallback* callback);
EXPORT(int) releaseStatic();
//EXPORT(enum LevelOfSupport) getLevelOfSupportFor(
//		const char* engineVersion, int engineAIInterfaceGeneratedVersion);


// skirmish AI related methods

EXPORT(const SSkirmishAILibrary*) loadSkirmishAILibrary(
	const char* const shortName,
	const char* const version
);
EXPORT(int) unloadSkirmishAILibrary(
	const char* const shortName,
	const char* const version
);
EXPORT(int) unloadAllSkirmishAILibraries();

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _INTERFACE_EXPORT_H
