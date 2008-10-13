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

// RAI stuff
#include "RAI.h"

#define MY_SHORT_NAME "RAI"
#define MY_VERSION "0.601"

std::map<int, CAIGlobalAI*> myAIs; // teamId -> AI map

Export(unsigned int) getInfo(struct InfoItem info[], unsigned int maxInfoItems) {

	unsigned int i = 0;

	if (i < maxInfoItems) {
		InfoItem ii = {SKIRMISH_AI_PROPERTY_SHORT_NAME, MY_SHORT_NAME, NULL};
		info[i] = ii;
		i++;
	}
	if (i < maxInfoItems) {
		InfoItem ii = {SKIRMISH_AI_PROPERTY_VERSION, MY_VERSION, NULL};
		info[i] = ii;
		i++;
	}
	if (i < maxInfoItems) {
		InfoItem ii = {SKIRMISH_AI_PROPERTY_NAME, "Reths Skirmish AI", NULL};
		info[i] = ii;
		i++;
	}
	if (i < maxInfoItems) {
		InfoItem ii = {SKIRMISH_AI_PROPERTY_DESCRIPTION, "This Skirmish AI supports most mods and plays decently.", NULL};
		info[i] = ii;
		i++;
	}
	if (i < maxInfoItems) {
		InfoItem ii = {SKIRMISH_AI_PROPERTY_URL, "http://spring.clan-sy.com/wiki/AI:RAI", NULL};
		info[i] = ii;
		i++;
	}
	if (i < maxInfoItems) {
		InfoItem ii = {SKIRMISH_AI_PROPERTY_LOAD_SUPPORTED, "no", NULL};
		info[i] = ii;
		i++;
	}

	// return the number of elements copied to info
	return i;
}

Export(enum LevelOfSupport) getLevelOfSupportFor(
		const char* engineVersionString, int engineVersionNumber,
		const char* aiInterfaceShortName, const char* aiInterfaceVersion) {

	if (strcmp(engineVersionString, ENGINE_VERSION_STRING) == 0 &&
			engineVersionNumber <= ENGINE_VERSION_NUMBER) {
		return LOS_Working;
	}

	return LOS_None;
}

Export(unsigned int) getOptions(struct Option options[], unsigned int max) {
	return 0;
}

Export(int) init(int teamId) {

	if (myAIs.count(teamId) > 0) {
		// the map already has an AI for this team.
		// raise an error, since it's probably a mistake if we're trying
		// to reinitialise a team that already had init() called on it.
		return -1;
	}

	// CAIGlobalAI is the Legacy C++ wrapper, cRAI is RAI
	myAIs[teamId] = new CAIGlobalAI(teamId, new cRAI());

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
