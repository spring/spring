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
#include "LegacyCpp/AIAI.h"
//#include "Game/GameVersion.h"
#include "CUtils/Util.h"

// AAI stuff
#include "AAI.h"

#include <map>

// skirmishAIId -> AI map
static std::map<int, CAIAI*> myAIs;

// callbacks for all the instances controlled by this Skirmish AI
static std::map<int, const struct SSkirmishAICallback*> skirmishAIId_callback;


/*
EXPORT(enum LevelOfSupport) getLevelOfSupportFor(
		const char* aiShortName, const char* aiVersion,
		const char* engineVersionString, int engineVersionNumber,
		const char* aiInterfaceShortName, const char* aiInterfaceVersion) {

	if (strcmp(engineVersionString, SpringVersion::GetFull().c_str()) == 0 &&
			engineVersionNumber <= ENGINE_VERSION_NUMBER) {
		return LOS_Working;
	}

	return LOS_None;
}
*/

EXPORT(int) init(int skirmishAIId, const struct SSkirmishAICallback* callback) {

	if (myAIs.count(skirmishAIId) > 0) {
		// the map has no AI for this skirmishAIId.
		// raise an error, since it's probably a mistake if we are trying to
		// release a skirmishAIId that's not initialized.
		return -1;
	}

	skirmishAIId_callback[skirmishAIId] = callback;

	// CAIAI is the Legacy C++ wrapper
	myAIs[skirmishAIId] = new CAIAI(new AAI());

	// signal: everything went ok
	return 0;
}

EXPORT(int) release(int skirmishAIId) {

	if (myAIs.count(skirmishAIId) == 0) {
		// no AI for this skirmishAIId, raise an error
		return -1;
	}

	delete myAIs[skirmishAIId];
	myAIs[skirmishAIId] = NULL;
	myAIs.erase(skirmishAIId);

	skirmishAIId_callback.erase(skirmishAIId);

	// signal: everything went ok
	return 0;
}

EXPORT(int) handleEvent(int skirmishAIId, int topic, const void* data) {

	if (skirmishAIId < 0) {
		// events sent to skirmishAIId -1 will allways be to the AI object itself,
		// not to a particular skirmishAIId.
	} else if (myAIs.count(skirmishAIId) > 0) {
		// allow the AI instance to handle the event.
		return myAIs[skirmishAIId]->handleEvent(topic, data);
	}

	// no AI for that skirmishAIId, so return error.
	return -1;
}



///////////////////////////////////////////////////////
// methods from here on are for AI internal use only //
///////////////////////////////////////////////////////

const char* aiexport_getVersion() {

	// this is a bit unclean, but should not be a problem
	const int skirmishAIId = skirmishAIId_callback.begin()->first;

	return skirmishAIId_callback[skirmishAIId]->SkirmishAI_Info_getValueByKey(skirmishAIId, SKIRMISH_AI_PROPERTY_VERSION);
}
