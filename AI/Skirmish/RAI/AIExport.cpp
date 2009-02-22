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
#include "ExternalAI/Interface/SSAILibrary.h"
#include "LegacyCpp/AIGlobalAI.h"
#include "Game/GameVersion.h"
#include "CUtils/Util.h"

// RAI stuff
#include "RAI.h"

#include <map>

// teamId -> AI map
static std::map<int, CAIGlobalAI*> myAIs;

static std::map<int, unsigned int> myOptionsSize;
static std::map<int, const char**> myOptionsKeys;
static std::map<int, const char**> myOptionsValues;

EXPORT(enum LevelOfSupport) getLevelOfSupportFor(int teamId,
		const char* engineVersionString, int engineVersionNumber,
		const char* aiInterfaceShortName, const char* aiInterfaceVersion) {

	if (strcmp(engineVersionString, SpringVersion::GetFull().c_str()) == 0 &&
			engineVersionNumber <= ENGINE_VERSION_NUMBER) {
		return LOS_Working;
	}

	return LOS_None;
}

EXPORT(int) init(int teamId,
		unsigned int infoSize,
		const char** infoKeys, const char** infoValues,
		unsigned int optionsSize,
		const char** optionsKeys, const char** optionsValues) {

	if (myAIs.count(teamId) > 0) {
		// the map already has an AI for this team.
		// raise an error, since it's probably a mistake if we're trying
		// to reinitialise a team that already had init() called on it.
		return -1;
	}

	util_setMyInfo(infoSize, infoKeys, infoValues, NULL, NULL);

	myOptionsSize[teamId] = optionsSize;
	myOptionsKeys[teamId] = optionsKeys;
	myOptionsValues[teamId] = optionsValues;

	// CAIGlobalAI is the Legacy C++ wrapper, cRAI is RAI
	myAIs[teamId] = new CAIGlobalAI(teamId, new cRAI());

	// signal: everything went ok
	return 0;
}

EXPORT(int) release(int teamId) {

	if (myAIs.count(teamId) == 0) {
		// the map has no AI for this team.
		// raise an error, since it's probably a mistake if we're trying to
		// release a team that's not initialized.
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


// methods from here on are for AI internal use only

const char* aiexport_getDataDir(bool absoluteAndWriteable) {

	static char* dd_ws_rel = NULL;
	static char* dd_ws_abs_w = NULL;

	if (absoluteAndWriteable) {
		if (dd_ws_abs_w == NULL) {
			// this is the writeable one, absolute
			const char* dd = util_getMyInfo(SKIRMISH_AI_PROPERTY_DATA_DIR);

			dd_ws_abs_w = (char*) calloc(strlen(dd) + 1 + 1, sizeof(char));
			STRCPY(dd_ws_abs_w, dd);
			STRCAT(dd_ws_abs_w, sPS);
		}
		return dd_ws_abs_w;
	} else {
		if (dd_ws_rel == NULL) {
			dd_ws_rel = util_allocStrCatFSPath(4,
					SKIRMISH_AI_DATA_DIR, "RAI", aiexport_getVersion(), "X");
			// remove the X, so we end up with a slash at the end
			if (dd_ws_rel != NULL) {
				dd_ws_rel[strlen(dd_ws_rel) -1] = '\0';
			}
		}
		return dd_ws_rel;
	}

	return NULL;
}
const char* aiexport_getVersion() {
	return util_getMyInfo(SKIRMISH_AI_PROPERTY_VERSION);
}

const char* aiexport_getMyOption(int teamId, const char* key) {
	return util_map_getValueByKey(
			myOptionsSize[teamId],
			myOptionsKeys[teamId], myOptionsValues[teamId],
			key);
}
