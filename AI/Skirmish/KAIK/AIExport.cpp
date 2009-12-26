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

#include "AIExport.h"

// AI interface stuff
#include "ExternalAI/Interface/SSkirmishAILibrary.h"
#include "ExternalAI/Interface/SSkirmishAICallback.h"
#include "LegacyCpp/AIGlobalAI.h"
#include "Game/GameVersion.h"

// KAIK stuff
#include "KAIK.h"

#include <map>

// teamId -> AI map
static std::map<int, CAIGlobalAI*> myAIs;

// callbacks for all the teams controlled by this Skirmish AI
static std::map<int, const struct SSkirmishAICallback*> teamId_callback;



EXPORT(enum LevelOfSupport) getLevelOfSupportFor(
	int teamId,
	const char* engineVersionString, int engineVersionNumber,
	const char* aiInterfaceShortName, const char* aiInterfaceVersion
) {
	const char* springVersion = SpringVersion::GetFull().c_str();
	const int cmp = strcmp(engineVersionString, springVersion);

	if (cmp == 0 && engineVersionNumber <= ENGINE_VERSION_NUMBER) {
		return LOS_Working;
	}

	return LOS_None;
}

EXPORT(int) init(int teamId, const struct SSkirmishAICallback* callback) {

	if (myAIs.count(teamId) > 0) {
		// the map already has an AI for this team.
		// raise an error, since it's probably a mistake if we're trying
		// to reinitialise a team that already had init() called on it.
		return -1;
	}

	teamId_callback[teamId] = callback;

	// CAIGlobalAI is the Legacy C++ wrapper
	myAIs[teamId] = new CAIGlobalAI(teamId, new CKAIK());

	// signal: everything went ok
	return 0;
}

EXPORT(int) release(int teamId) {

	if (myAIs.count(teamId) == 0) {
		// no AI for this team, raise an error
		return -1;
	}

	delete myAIs[teamId];
	myAIs[teamId] = NULL;
	myAIs.erase(teamId);

	// signal: everything went ok
	return 0;
}

EXPORT(int) handleEvent(int teamId, int topic, const void* data) {

	if (teamId < 0) {
		// events sent to team -1 will always be to the AI object itself,
		// not to a particular team.
	} else if (myAIs.count(teamId) > 0) {
		// allow the AI instance to handle the event.
		return myAIs[teamId]->handleEvent(topic, data);
	}

	// no AI for that team, so return error.
	return -1;
}



///////////////////////////////////////////////////////
// methods from here on are for AI internal use only //
///////////////////////////////////////////////////////

const char* aiexport_getVersion(int teamId) {
	return teamId_callback[teamId]->Clb_SkirmishAI_Info_getValueByKey(teamId, SKIRMISH_AI_PROPERTY_VERSION);
}
