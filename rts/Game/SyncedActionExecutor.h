/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SYNCED_ACTION_EXECUTOR_H
#define SYNCED_ACTION_EXECUTOR_H

#include "Action.h"
#include "IActionExecutor.h"

#include <string>


class SyncedAction
{
public:
	SyncedAction(const Action& action, int playerID)
		: action(action)
		, playerID(playerID)
	{}

	/**
	 * Returns the action arguments.
	 */
	const std::string& GetArgs() const { return action.extra; }

	/**
	 * Returns the normalized key symbol.
	 */
	int GetPlayerID() const { return playerID; }

private:
	const Action& action;
	int playerID;
};

class ISyncedActionExecutor : public IActionExecutor<SyncedAction, true>
{
protected:
	ISyncedActionExecutor(const std::string& command, bool cheatRequired = false)
		: IActionExecutor<SyncedAction, true>(command, cheatRequired)
	{}

public:
	virtual ~ISyncedActionExecutor() {}
};

#endif // SYNCED_ACTION_EXECUTOR_H
