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

#include <map>

// AI interface stuff
#include "ExternalAI/Interface/SSAILibrary.h"
#include "LegacyCpp/AI.h"
#include "LegacyCpp/AIGlobalAI.h"
#include "Game/GameVersion.h"

// NTai stuff
#include "CNTai.h"


static std::map<int, CAIGlobalAI*> myAIs; // teamId -> AI map

static unsigned int myInfoSize;
static const char** myInfoKeys;
static const char** myInfoValues;

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

	myInfoSize = infoSize;
	myInfoKeys = infoKeys;
	myInfoValues = infoValues;

	myOptionsSize[teamId] = optionsSize;
	myOptionsKeys[teamId] = optionsKeys;
	myOptionsValues[teamId] = optionsValues;

	// CAIGlobalAI is the Legacy C++ wrapper, CGlobalAI is KAIK
	myAIs[teamId] = new CAIGlobalAI(teamId, new ntai::CNTai());

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

static const char* util_map_getValueByKey(
		unsigned int infoSize,
		const char** infoKeys, const char** infoValues,
		const char* key) {

	const char* value = NULL;

	unsigned int i;
	for (i = 0; i < infoSize; i++) {
		if (strcmp(infoKeys[i], key) == 0) {
			value = infoValues[i];
			break;
		}
	}

	return value;
}

const char* aiexport_getMyInfo(const char* key) {
	return util_map_getValueByKey(myInfoSize, myInfoKeys, myInfoValues, key);
}
const char* aiexport_getDataDir() {

//	static char* ddWithSlash = NULL;
//
//	if (ddWithSlash == NULL) {
//		const char* dd = aiexport_getMyInfo(SKIRMISH_AI_PROPERTY_DATA_DIR);
//
//		ddWithSlash = (char*) calloc(strlen(dd) + 1 + 1, sizeof(char));
//		strcpy(ddWithSlash, dd);
//		strcat(ddWithSlash, "/");
//	}
//
//	return ddWithSlash;
	return aiexport_getMyInfo(SKIRMISH_AI_PROPERTY_DATA_DIR);
}
const char* aiexport_getVersion() {
	return aiexport_getMyInfo(SKIRMISH_AI_PROPERTY_VERSION);
}

const char* aiexport_getMyOption(int teamId, const char* key) {
	return util_map_getValueByKey(
			myOptionsSize[teamId],
			myOptionsKeys[teamId], myOptionsValues[teamId],
			key);
}
