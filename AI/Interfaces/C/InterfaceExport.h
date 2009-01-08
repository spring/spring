/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _INTERFACEEXPORT_H
#define _INTERFACEEXPORT_H

#if	!defined BUILDING_AI_INTERFACE
#error BUILDING_AI_INTERFACE should be defined when building AI interfaces
#endif
#if	defined BUILDING_AI
#error BUILDING_AI should not be defined when building AI interfaces
#endif
#if	defined SYNCIFY
#error SYNCIFY should not be defined when building AI interfaces
#endif

#include "ExternalAI/Interface/aidefines.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "ExternalAI/Interface/ELevelOfSupport.h"

struct SSAILibrary;
struct SStaticGlobalData;

// for a list of the functions that have to be exported,
// see struct SAIInterfaceLibrary in:
// "rts/ExternalAI/Interface/SAIInterfaceLibrary.h"


// static AI interface library functions

EXPORT(int) initStatic(
		unsigned int infoSize,
		const char** infoKeys, const char** infoValues,
		const SStaticGlobalData* staticGlobalData);
EXPORT(int) releaseStatic();
EXPORT(enum LevelOfSupport) getLevelOfSupportFor(
		const char* engineVersion, int engineAIInterfaceGeneratedVersion);


// skirmish AI related methods

EXPORT(const struct SSAILibrary*) loadSkirmishAILibrary(
		unsigned int infoSize,
		const char** infoKeys, const char** infoValues);
EXPORT(int) unloadSkirmishAILibrary(
		unsigned int infoSize,
		const char** infoKeys, const char** infoValues);
EXPORT(int) unloadAllSkirmishAILibraries();

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _INTERFACEEXPORT_H
