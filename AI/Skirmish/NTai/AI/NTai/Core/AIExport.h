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

#ifndef _AIEXPORT_H
#define _AIEXPORT_H

// check if build system uses correct defines
#if	!defined BUILDING_AI
	#error BUILDING_AI should be defined when building AIs
#endif
#if	defined BUILDING_AI_INTERFACE
	#error BUILDING_AI_INTERFACE should not be defined when building AIs
#endif

#include "ExternalAI/Interface/aidefines.h"
#include "ExternalAI/Interface/ELevelOfSupport.h"

// for a list of the functions that have to be exported,
// see struct SSAILibrary in "ExternalAI/Interface/SSAILibrary.h"

// static AI library methods (optional to implement)
EXPORT(enum LevelOfSupport) getLevelOfSupportFor(int teamId,
		const char* engineVersionString, int engineVersionNumber,
		const char* aiInterfaceShortName, const char* aiInterfaceVersion);

// team instance functions
EXPORT(int) init(int teamId,
		unsigned int infoSize,
		const char** infoKeys, const char** infoValues,
		unsigned int optionsSize,
		const char** optionsKeys, const char** optionsValues);
EXPORT(int) release(int teamId);
EXPORT(int) handleEvent(int teamId, int topic, const void* data);

#endif // _AIEXPORT_H

