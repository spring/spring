/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>

#include "SyncedActionExecutor.h"
#include "Action.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/LogOutput.h"


void ISyncedActionExecutor::ExecuteAction(const Action& action, int playerID) const
{
	//assert(action.command == GetCommand());

	if (IsCheatRequired() && !gs->cheatEnabled) {
		logOutput.Print("Synced chat command /" + GetCommand() + " requires /cheat");
	} else {
		Execute(action.extra, playerID);
	}
}
