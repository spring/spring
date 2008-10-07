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

#include "InterfaceExport.h"

#include "Interface.h"

#include "ExternalAI/Interface/SAIInterfaceLibrary.h"
/*
#include "ExternalAI/Interface/ELevelOfSupport.h"
#include "ExternalAI/Interface/SSAILibrary.h"
#include "ExternalAI/Interface/SGAILibrary.h"
*/

/*
If we do not have an init() method, then we would instead pass
an event InitEvent to handleEvent. However, we would have to make
handleEvent have to wait for an InitEvent as a special case, since
the team in question would not yet exist. 

Therefore, the handleEvent code would look like this:
[code]
SHARED_EXPORT int handleEvent(int team, int eventID, void* event) {
    if (eventID == INIT_EVENT) {
        ais[team] = CAIObject();
    }
    if (ais.count(team) > 0){
        // allow the AI instance to handle the event.
        return ais[team].handleEvent(eventID, event);
    }
    // no ai with value, so return error.
    else return -1;
}
[/code]
Advantages:
* All events to AI go through handleEvent.
* People don't get confused and start adding bananaSplitz() functions.

Disadvantages:
* We need to check for INIT_EVENT before *every message*.
* These events will happen only once per game -- after that the check becomes a necessary waste of time.
* Handling events is no longer about getting the right object to deal with an event, it also includes initialising object properly.

I understand that we want to keep the interface simple. In fact,
I think we should keep it as minimal as possible, and ideally everything
would go through handleEvent. Practically though, it does not make sense
to do this: we'll be wasting our own time for no good reason in the
case of initialisation. 

The (in my opinion much cleaner) alternative is the one I've implemented.
Advantages:
* One function that initialises a team before everything is passed to handlEvent
* No redundant if statements.
* Simple design.

Disadvantage:
* People might start adding other functions to the interface.

I don't think that the disadvantage is a real one: it's pretty standard to 
see initialisation as a special case. It's pretty clear that everything
else goes through handleEvent. 

The advantage is clear: a more efficient, simpler design. Of course, you could
argue that the efficiency is nominal, one extra if per event is very little cost,
and granted, that's true; but this doesn't change the fact that we're checking
for a special case that we know only happens once at the beginning of the game, before
every single event after.

Of course, we still need an INIT_EVENT, since initialising the existance of a team
member is not the same as initialising its state.

You might also argue that we do this check in the handleEvent switch within each
team. This is true, although the difference there is that a switch is translated
to address lookups and so there is no increase in cost if there are more switch
cases. 
*/

// AI Interface vars
/*
const char* const myProperties[][2] = {
	{AI_INTERFACE_PROPERTY_SHORT_NAME, "C"},
	{AI_INTERFACE_PROPERTY_VERSION, "0.1"},
	{AI_INTERFACE_PROPERTY_NAME, "C & C++"},
	{AI_INTERFACE_PROPERTY_DESCRIPTION, "This AI Interface library is needed for Skirmish and Group AIs written in C or C++."},
	{AI_INTERFACE_PROPERTY_URL, "http://sy.spring.com/Wiki/CInterface"},
	{"supportedLanguages", "C, C++"}
};
const int numMyProperties = 6;
*/


/*
// Skirmish AI vars
const struct SSAISpecifyer* mySAISpecifyers = NULL;
const int numMySAISpecifyers;

const struct SSAILibrary* mySAILibraries;
const int numMySAILibraries;


// Group AI vars
const struct SGAISpecifyer* myGAISpecifyers = NULL;
const int numMyGAISpecifyers;

const struct SGAILibrary* myGAILibraries;
const int numMyGAILibraries;
*/

CInterface myInterface;

/*
#include "stdio.h"
*/
//#include "<iostream>"

Export(enum LevelOfSupport) getLevelOfSupportFor(
		const char* engineVersion, int engineAIInterfaceGeneratedVersion) {
	return myInterface.GetLevelOfSupportFor(engineVersion, engineAIInterfaceGeneratedVersion);
}

Export(int) getInfos(InfoItem infos[], unsigned int max) {
	return myInterface.GetInfos(infos, max);
}


// skirmish AI methods
/*
Export(int) getSkirmishAISpecifyers(struct SSAISpecifyer* sAISpecifyers, int max) {
	return myInterface.GetSkirmishAISpecifyers(sAISpecifyers, max);
}
Export(const struct SSAILibrary*) loadSkirmishAILibrary(const struct SSAISpecifyer* const sAISpecifyer) {
	return myInterface.LoadSkirmishAILibrary(sAISpecifyer);
}
*/
Export(const struct SSAILibrary*) loadSkirmishAILibrary(const struct InfoItem infos[], unsigned int numInfos) {
	return myInterface.LoadSkirmishAILibrary(infos, numInfos);
}
Export(int) unloadSkirmishAILibrary(const struct SSAISpecifyer* const sAISpecifyer) {
	return myInterface.UnloadSkirmishAILibrary(sAISpecifyer);
}
Export(int) unloadAllSkirmishAILibraries() {
	return myInterface.UnloadAllSkirmishAILibraries();
}



// group AI methods
/*
Export(int) getGroupAISpecifyers(struct SGAISpecifyer* gAISpecifyers, int max) {
	return myInterface.GetGroupAISpecifyers(gAISpecifyers, max);
}
Export(const struct SGAILibrary*) loadGroupAILibrary(const struct SGAISpecifyer* const gAISpecifyer) {
	return myInterface.LoadGroupAILibrary(gAISpecifyer);
}
*/
Export(const struct SGAILibrary*) loadGroupAILibrary(const struct InfoItem infos[], unsigned int numInfos) {
	return myInterface.LoadGroupAILibrary(infos, numInfos);
}
Export(int) unloadGroupAILibrary(const struct SGAISpecifyer* const gAISpecifyer) {
	return myInterface.UnloadGroupAILibrary(gAISpecifyer);
}
Export(int) unloadAllGroupAILibraries() {
	return myInterface.UnloadAllGroupAILibraries();
}

