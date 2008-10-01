#include "AIExport.h"

#include <map>

#include "ExternalAI/Interface/LegacyCppWrapper/AI.h"
#include "ExternalAI/Interface/LegacyCppWrapper/AIGlobalAI.h"
#include "ExternalAI/Interface/SSAILibrary.h"

/*
If we do not have an init() method, then we would instead pass
an event InitEvent to handleEvent. However, we would have to make
handleEvent have to wait for an InitEvent as a special case, since
the team in question would not yet exist. 

Therefore, the handleEvent code would look like this:
[code]
DLL_EXPORT int handleEvent(int team, int eventID, void* event) {
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

std::map<int, CAI*> ais;

/*
DLL_EXPORT int version() {
    return 1;
}
*/

// Since this is a C interface, we can only be told by the engine
// to set up an AI with the number team that indicates a receiver
// of any handleEvent() call.
Export(int) init(int teamId) {
	
    if (ais.count(teamId) > 0) {
		// the map already has an AI for this team.
		// raise an error, since it's probably a mistake if we're trying
		// reinitialise a team that's already had init() called on it.
        return -1;
    }
	
    //TODO:
    // Change the line below so that CAI is 
    // your AI, which should be a subclass of CAI that
    // overrides the handleEvent() method.
    ais[teamId] = new CAI(teamId, NULL);
}

Export(int) release(int teamId) {
	
    if (ais.count(teamId) == 0) {
		// the map has no AI for this team.
		// raise an error, since it's probably a mistake if we're trying to
		// release a team that's not initialized.
        return -1;
    }
	
    delete ais[teamId];
	ais.erase(teamId);
	
	return 0;
}

Export(int) handleEvent(int teamId, int topic, const void* data) {
	
	if (ais.count(teamId) > 0) {
        // allow the AI instance to handle the event.
        return ais[teamId]->handleEvent(topic, data);
    } else {
		// events sent to team -1 will always be to the AI object itself,
		// not to a particular team.
		
		// no ai with value, so return error.
		return -1;
	}
}

