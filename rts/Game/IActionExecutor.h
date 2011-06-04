/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_ACTION_EXECUTOR_H
#define I_ACTION_EXECUTOR_H

//#include "Action.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/LogOutput.h"

#include <string>

template<class action_t, bool synced_v> class IActionExecutor
{
protected:
	IActionExecutor(const std::string& command, bool cheatRequired = false)
		: command(command)
		, cheatRequired(cheatRequired)
	{}

public:
	virtual ~IActionExecutor() {}

	/**
	 * Returns the command string that is unique for this executor.
	 */
	const std::string& GetCommand() const { return command; }

	/**
	 * Returns whether this executor handles synced or unsynced commands.
	 */
	bool IsSynced() const { return synced_v; }

	/**
	 * Returns the command string that is unique for this executor.
	 */
	bool IsCheatRequired() const { return cheatRequired; }

	/**
	 * Executes one instance of an action of this type.
	 * Does a few checks internally, and then calls Execute(args).
	 */
	void ExecuteAction(const action_t& action) const {
		//assert(action.GetAction().command == GetCommand());

		if (IsCheatRequired() && !gs->cheatEnabled) {
			logOutput.Print("Chat command /%s (%s) requires /cheat",
					GetCommand().c_str(),
					(IsSynced() ? "synced" : "unsynced"));
		} else {
			Execute(action);
		}
	}

protected:
	/**
	 * Executes one instance of an action of this type.
	 */
	virtual void Execute(const action_t& action) const = 0;

private:
	std::string command;
	bool cheatRequired;
};

#endif // I_ACTION_EXECUTOR_H
