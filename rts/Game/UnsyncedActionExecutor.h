/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNSYNCED_ACTION_EXECUTOR_H
#define UNSYNCED_ACTION_EXECUTOR_H

#include "Action.h"

#include <string>


class UnsyncedAction
{
public:
	UnsyncedAction(const Action& action, unsigned int key, bool repeat)
		: action(action)
		, key(key)
		, repeat(repeat)
	{}

	/**
	 * Returns the action arguments.
	 */
	const std::string& GetArgs() const { return action.extra; }

	/**
	 * Returns the normalized key symbol.
	 */
	unsigned int GetKey() const { return key; }

	/**
	 * Returns whether the action is to be executed repeatedly.
	 */
	bool IsRepeat() const { return repeat; }

	const Action& GetInnerAction() const { return action; }

private:
	const Action& action;
	unsigned int key;
	bool repeat;
};


class IUnsyncedActionExecutor
{
protected:
	IUnsyncedActionExecutor(const std::string& command, bool cheatRequired = false)
		: command(command)
		, cheatRequired(cheatRequired)
	{}

public:
	virtual ~IUnsyncedActionExecutor() {}

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
	void ExecuteAction(const UnsyncedAction& action) const;

protected:
	/**
	 * Executes one instance of an action of this type.
	 */
	virtual void Execute(const UnsyncedAction& action) const = 0;

private:
	std::string command;
	bool cheatRequired;
};

#endif // UNSYNCED_ACTION_EXECUTOR_H
