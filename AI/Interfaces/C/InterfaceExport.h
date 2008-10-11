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

#ifdef	__cplusplus
extern "C" {
#endif

#include "ExternalAI/Interface/ELevelOfSupport.h"
struct InfoItem;
struct SSAISpecifier;
struct SSAILibrary;
struct SGAISpecifier;
struct SGAILibrary;
struct SStaticGlobalData;

// for a list of the functions that have to be exported,
// see struct SAIInterfaceLibrary in "ExternalAI/Interface/SAIInterfaceLibrary.h"

// static AI interface library functions

Export(int) initStatic(const SStaticGlobalData* staticGlobalData);
Export(int) releaseStatic();
Export(enum LevelOfSupport) getLevelOfSupportFor(
		const char* engineVersion, int engineAIInterfaceGeneratedVersion);
Export(unsigned int) getInfo(struct InfoItem info[], unsigned int maxInfoItems);
	
// skirmish AI methods
//Export(int) getSkirmishAISpecifiers(struct SSAISpecifier* sAISpecifiers, int max);
//Export(const struct SSAILibrary*) loadSkirmishAILibrary(const struct SSAISpecifier* const sAISpecifier);
Export(const struct SSAILibrary*) loadSkirmishAILibrary(const struct InfoItem info[], unsigned int numInfoItems);
Export(int) unloadSkirmishAILibrary(const struct SSAISpecifier* const sAISpecifier);
Export(int) unloadAllSkirmishAILibraries();
	
// group AI methods
//Export(int) getGroupAISpecifiers(struct SGAISpecifier* gAISpecifiers, int max);
//Export(const struct SGAILibrary*) loadGroupAILibrary(const struct SGAISpecifier* const gAISpecifier);
Export(const struct SGAILibrary*) loadGroupAILibrary(const struct InfoItem info[], unsigned int numInfoItems);
Export(int) unloadGroupAILibrary(const struct SGAISpecifier* const gAISpecifier);
Export(int) unloadAllGroupAILibraries();

#ifdef	__cplusplus
}
#endif

#endif	/* _INTERFACEEXPORT_H */

