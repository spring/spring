/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_ACTION_EXECUTOR_H
#define I_ACTION_EXECUTOR_H

#include "Action.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/LogOutput.h"

#include <string>

class IAction
{
protected:
	IAction(const Action& action)
		: action(action)
	{}
	virtual ~IAction() {}

public:
	/**
	 * Returns the action arguments.
	 */
	const std::string& GetArgs() const { return action.extra; }

	const Action& GetInnerAction() const { return action; }

private:
	const Action& action;
};

template<class action_t, bool synced_v>
class IActionExecutor
{
protected:
	IActionExecutor(const std::string& command,
			const std::string& description, bool cheatRequired = false)
		: command(command)
		, description(description)
		, cheatRequired(cheatRequired)
	{}
	virtual ~IActionExecutor() {}

public:
	/**
	 * Returns the command string that is unique for this executor.
	 */
	const std::string& GetCommand() const { return command; }

	/**
	 * Returns a human readable description of the command handled by this
	 * executor.
	 * This text will eventually be shown to end-users of the engine (gamers).
	 */
	const std::string& GetDescription() const { return description; }

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
	void ExecuteAction(const action_t& action) const;

protected:
	void SetDescription(const std::string& description) {
		this->description = description;
	}

	/**
	 * Sets a bool according to the value encoded in a string.
	 * The conversion works like this:
	 * - ""  -> toggle-value
	 * - "0" -> false
	 * - "1" -> true
	 */
	static void SetBoolArg(bool& container, const std::string& newValue);

	/**
	 * Logs the enabled/disabled status of a sub-system of the engine.
	 */
	static void LogSystemStatus(const std::string& system, bool status);

private:
	/**
	 * Executes one instance of an action of this type.
	 */
	virtual void Execute(const action_t& action) const = 0;

	std::string command;
	std::string description;
	bool cheatRequired;
};



/*
 * Because this is a template enabled class,
 * the implementations have to be in the same file.
 */

template<class action_t, bool synced_v>
void IActionExecutor<action_t, synced_v>::ExecuteAction(const action_t& action) const {
	//assert(action.GetAction().command == GetCommand());

	if (IsCheatRequired() && !gs->cheatEnabled) {
		logOutput.Print("Chat command /%s (%s) requires /cheat",
				GetCommand().c_str(),
				(IsSynced() ? "synced" : "unsynced"));
	} else {
		Execute(action);
	}
}

template<class action_t, bool synced_v>
void IActionExecutor<action_t, synced_v>::SetBoolArg(bool& container, const std::string& newValue) {

	if (newValue.empty())  {
		// toggle
		container = !container;
	} else {
		// set
		const int num = atoi(newValue.c_str());
		container = (num != 0);
	}
}

template<class action_t, bool synced_v>
void IActionExecutor<action_t, synced_v>::LogSystemStatus(const std::string& system, bool status) {

	logOutput.Print("%s is %s!", system.c_str(),
			(status ? "enabled" : "disabled"));
}


#endif // I_ACTION_EXECUTOR_H
