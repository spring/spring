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

#include "ExternalAI/Interface/aidefines.h"

#ifdef	__cplusplus
extern "C" {
#endif

#include "ExternalAI/Interface/ELevelOfSupport.h"
struct InfoItem;
struct SSAISpecifyer;
struct SSAILibrary;
struct SGAISpecifyer;
struct SGAILibrary;

// for a list of the functions that have to be exported,
// see struct SAIInterfaceLibrary in "ExternalAI/Interface/SAIInterfaceLibrary.h"

// static AI interface library functions
Export(enum LevelOfSupport) getLevelOfSupportFor(
		const char* engineVersion, int engineAIInterfaceGeneratedVersion);
Export(int) getInfos(struct InfoItem infos[], unsigned int max);
	
// skirmish AI methods
//Export(int) getSkirmishAISpecifyers(struct SSAISpecifyer* sAISpecifyers, int max);
//Export(const struct SSAILibrary*) loadSkirmishAILibrary(const struct SSAISpecifyer* const sAISpecifyer);
Export(const struct SSAILibrary*) loadSkirmishAILibrary(const struct InfoItem infos[], unsigned int numInfos);
Export(int) unloadSkirmishAILibrary(const struct SSAISpecifyer* const sAISpecifyer);
Export(int) unloadAllSkirmishAILibraries();
	
// group AI methods
//Export(int) getGroupAISpecifyers(struct SGAISpecifyer* gAISpecifyers, int max);
//Export(const struct SGAILibrary*) loadGroupAILibrary(const struct SGAISpecifyer* const gAISpecifyer);
Export(const struct SGAILibrary*) loadGroupAILibrary(const struct InfoItem infos[], unsigned int numInfos);
Export(int) unloadGroupAILibrary(const struct SGAISpecifyer* const gAISpecifyer);
Export(int) unloadAllGroupAILibraries();

#ifdef	__cplusplus
}
#endif

#endif	/* _INTERFACEEXPORT_H */

