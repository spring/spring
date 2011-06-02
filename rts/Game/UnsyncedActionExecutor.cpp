/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>

#include "UnsyncedActionExecutor.h"
#include "Action.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/LogOutput.h"


void IUnsyncedActionExecutor::ExecuteAction(const UnsyncedAction& action) const
{
	//assert(action.GetAction().command == GetCommand());

	if (IsCheatRequired() && !gs->cheatEnabled) {
		logOutput.Print("Chat command /%s (unsynced) requires /cheat", GetCommand().c_str());
	} else {
		Execute(action);
	}
}

