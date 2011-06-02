/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SYNCED_ACTION_EXECUTOR_H
#define SYNCED_ACTION_EXECUTOR_H

#include "Action.h"

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
	 * Returns the player ID.
	 */
	int GetPlayerID() const { return playerID; }

private:
	const Action& action;
	int playerID;
};

class ISyncedActionExecutor
{
protected:
	ISyncedActionExecutor(const std::string& command, bool cheatRequired = false)
		: command(command)
		, cheatRequired(cheatRequired)
	{}

public:
	virtual ~ISyncedActionExecutor() {}

	/**
	 * Returns the command string that is unique for this executor.
	 */
	const std::string& GetCommand() const { return command; }

	/**
	 * Returns the command string that is unique for this executor.
	 */
	bool IsCheatRequired() const { return cheatRequired; }

	/**
	 * Executes one instance of an action of this type.
	 * Does a few checks internally, and then calls Execute(args).
	 */
	void ExecuteAction(const SyncedAction& action) const;

protected:
	/**
	 * Executes one instance of an action of this type.
	 */
	virtual void Execute(const SyncedAction& action) const = 0;

private:
	std::string command;
	bool cheatRequired;
};

#endif // SYNCED_ACTION_EXECUTOR_H
