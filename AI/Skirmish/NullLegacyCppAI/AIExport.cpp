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

// NullLegacyCppAI stuff
#include "TestGlobalAI.h"


std::map<int, CAIGlobalAI*> myAIs; // teamId -> AI map


Export(int) init(int teamId,
		const struct InfoItem info[], unsigned int numInfoItems) {
	
    if (myAIs.count(teamId) > 0) {
		// the map already has an AI for this team.
		// raise an error, since it's probably a mistake if we're trying
		// to reinitialise a team that already had init() called on it.
        return -1;
    }
	
    // CAIGlobalAI is the Legacy C++ wrapper, TestGlobalAI is NullLegacyCppAI
    myAIs[teamId] = new CAIGlobalAI(teamId, new TestGlobalAI());
	
	// signal: ok
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
	
	// signal: ok
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

