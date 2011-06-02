/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>

#include "SyncedActionExecutor.h"
#include "Action.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/LogOutput.h"


void ISyncedActionExecutor::ExecuteAction(const SyncedAction& action) const
{
	//assert(action.command == GetCommand());

	if (IsCheatRequired() && !gs->cheatEnabled) {
		logOutput.Print("Chat command /%s (synced) requires /cheat", GetCommand().c_str());
	} else {
		Execute(action);
	}
}
