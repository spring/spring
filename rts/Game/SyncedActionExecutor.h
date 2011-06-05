/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SYNCED_ACTION_EXECUTOR_H
#define SYNCED_ACTION_EXECUTOR_H

#include "IActionExecutor.h"

#include <string>

class Action;


class SyncedAction : public IAction
{
public:
	SyncedAction(const Action& action, int playerID)
		: IAction(action)
		, playerID(playerID)
	{}

	/**
	 * Returns the normalized key symbol.
	 */
	int GetPlayerID() const { return playerID; }

private:
	int playerID;
};

class ISyncedActionExecutor : public IActionExecutor<SyncedAction, true>
{
protected:
	ISyncedActionExecutor(const std::string& command, const std::string& description, bool cheatRequired = false)
		: IActionExecutor<SyncedAction, true>(command, description, cheatRequired)
	{}

public:
	virtual ~ISyncedActionExecutor() {}
};

#endif // SYNCED_ACTION_EXECUTOR_H
