/*
	Copyright 2008  Nicolas Wu

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

	@author Nicolas Wu
	@author Robin Vobruba <hoijui.quaero@gmail.com>
*/

#ifndef AAI_AIEXPORT_H
#define AAI_AIEXPORT_H

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
#include "ExternalAI/Interface/ELevelOfSupport.h"

struct SSkirmishAICallback;

// for a list of the functions that have to be exported,
// see struct SSkirmishAILibrary in "ExternalAI/Interface/SSkirmishAILibrary.h"

// static AI library methods (optional to implement)
/*EXPORT(enum LevelOfSupport) getLevelOfSupportFor(
		const char* aiShortName, const char* aiVersion,
		const char* engineVersionString, int engineVersionNumber,
		const char* aiInterfaceShortName, const char* aiInterfaceVersion);
*/
// instance functions
EXPORT(int) init(int skirmishAIId, const struct SSkirmishAICallback* callback);
EXPORT(int) release(int skirmishAIId);
EXPORT(int) handleEvent(int skirmishAIId, int topic, const void* data);

// methods from here on are for AI internal use only
const char* aiexport_getVersion();

#endif // _AIEXPORT_H

