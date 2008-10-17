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
#include "ExternalAI/Interface/LegacyCppWrapper/AI.h"
#include "ExternalAI/Interface/LegacyCppWrapper/AIGlobalAI.h"

// KAIK stuff
#include "GlobalAI.h"


std::map<int, CAIGlobalAI*> myAIs; // teamId -> AI map
std::vector<InfoItem> myInfo;


Export(unsigned int) getInfo(int teamId,
		struct InfoItem info[], unsigned int maxInfoItems) {

	unsigned int i = 0;

	// initialize the myInfo
	if (myInfo.empty()) {
		InfoItem ii_0 = {SKIRMISH_AI_PROPERTY_SHORT_NAME, "KAIK", NULL}; myInfo.push_back(ii_0);
		InfoItem ii_1 = {SKIRMISH_AI_PROPERTY_VERSION, "0.13", NULL}; myInfo.push_back(ii_1);
		InfoItem ii_2 = {SKIRMISH_AI_PROPERTY_NAME, "Kloots Skirmish AI", NULL}; myInfo.push_back(ii_2);
		InfoItem ii_3 = {SKIRMISH_AI_PROPERTY_DESCRIPTION, "This Skirmish AI supports most TA based mods and plays decently.", NULL}; myInfo.push_back(ii_3);
		InfoItem ii_4 = {SKIRMISH_AI_PROPERTY_URL, "http://spring.clan-sy.com/wiki/AI:KAIK", NULL}; myInfo.push_back(ii_4);
		InfoItem ii_5 = {SKIRMISH_AI_PROPERTY_LOAD_SUPPORTED, "no", NULL}; myInfo.push_back(ii_5);
	}

	// copy myInfo to the argument container info
	std::vector<InfoItem>::const_iterator inf;
	for (inf=myInfo.begin(); inf!=myInfo.end() && i < maxInfoItems; inf++) {
		info[i] = *inf;
		i++;
	}

	// return the number of elements copied to info 
	return i;
}

Export(enum LevelOfSupport) getLevelOfSupportFor(int teamId,
		const char* engineVersionString, int engineVersionNumber,
		const char* aiInterfaceShortName, const char* aiInterfaceVersion) {

	if (strcmp(engineVersionString, ENGINE_VERSION_STRING) == 0 &&
			engineVersionNumber <= ENGINE_VERSION_NUMBER) {
		return LOS_Working;
	}

	return LOS_None;
}

Export(unsigned int) getOptions(int teamId,
		struct Option options[], unsigned int maxOptions) {
	return 0;
}

Export(int) init(int teamId,
		const struct InfoItem info[], unsigned int numInfoItems) {

	if (myAIs.count(teamId) > 0) {
		// the map already has an AI for this team.
		// raise an error, since it's probably a mistake if we're trying
		// to reinitialise a team that already had init() called on it.
		return -1;
	}

	// CAIGlobalAI is the Legacy C++ wrapper, CGlobalAI is KAIK
	myAIs[teamId] = new CAIGlobalAI(teamId, new CGlobalAI());

	// signal: everything went ok
	return 0;
}

Export(int) release(int teamId) {

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

Export(int) handleEvent(int teamId, int topic, const void* data) {

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
