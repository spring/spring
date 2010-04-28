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

/*
If we do not have an init() method, then we would instead pass
an event InitEvent to handleEvent. However, we would have to make
handleEvent have to wait for an InitEvent as a special case, since
the team in question would not yet exist.

Therefore, the handleEvent code would look like this:
[code]
EXPORT(int) handleEvent(int teamId, int topic, const void* data) {
	if (topic == INIT_EVENT) {
		myAIs[teamId] = CAIObject();
	}
	if (myAIs.count(teamId) > 0) {
		// allow the AI instance to handle the event.
		return myAIs[teamId].handleEvent(topic, data);
	} else {
		// no AI for that team, so return error.
		return -1;
	}
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

Of course, we still need an EVENT_INIT, since initialising the existance of a team
member is not the same as initialising its state.

You might also argue that we do this check in the handleEvent switch within each
team. This is true, although the difference there is that a switch is translated
to address lookups and so there is no increase in cost if there are more switch
cases.

The same issues and reasonins described here for init() and EVENT_INIT applies
to release() and EVENT_RELEASE.
*/

//#include "ExternalAI/Interface/SSkirmishAICallback.h"

#include "AIExport.h"

/*EXPORT(int) init(int teamId, const struct SSkirmishAICallback* callback) {

	// TODO: do something

	// signal: ok
    return 0;
}*/

EXPORT(int) handleEvent(int teamId, int topic, const void* data) {

	// TODO: do something

	// signal: ok
	return 0;
}
