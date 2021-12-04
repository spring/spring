/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_GAME_COMMANDS_H
#define I_GAME_COMMANDS_H

#include "System/StringUtil.h"

// These two are required for the destructors
#include "SyncedActionExecutor.h"
#include "UnsyncedActionExecutor.h"
#include "WordCompletion.h"

#include <map>
#include <string>
#include <stdexcept>


template<class actionExecutor_t>
class IGameCommands
{
protected:
	IGameCommands() {}
	virtual ~IGameCommands();

public:
	typedef std::map<std::string, actionExecutor_t*> actionExecutorsMap_t;

	/**
	 * Registers the default action-executors for chat commands.
	 * These are all the ones that are not logically tied to a specific
	 * part/sub-module of the engine (for example rendering related switches).
	 * @see AddActionExecutor
	 */
	virtual void AddDefaultActionExecutors() = 0;

	/**
	 * Registers a new action-executor for a chat command.
	 * @param executor has to be new'ed, will be delete'ed internally.
	 * @see RemoveActionExecutor
	 */
	void AddActionExecutor(actionExecutor_t* executor);

	/**
	 * Deregisters an action-executor for a chat command.
	 * The action-executor corresponding to the given command
	 * (case-insensitive) will be removed and delete'ed internally.
	 * @param command used to lookup the action-executor to be removed.
	 * @see AddActionExecutor
	 */
	void RemoveActionExecutor(const std::string& command);

	/**
	 * Deregisters all currently registered action-executor for chat commands.
	 * @see RemoveActionExecutor
	 */
	void RemoveAllActionExecutors();

	/**
	 * Returns the action-executor for the given command (case-insensitive).
	 * @param command used to lookup the action-executor to be removed.
	 * @return the action-executor for the given command, or NULL, if none is
	 *   registered.
	 */
	const actionExecutor_t* GetActionExecutor(const std::string& command) const;

	/**
	 * Returns the map of currently registered lower-case commands with their
	 * respective action-executors.
	 */
	const actionExecutorsMap_t& GetActionExecutors() const { return actionExecutors; }

private:
	// XXX maybe use a hash_map here, for faster lookup
	actionExecutorsMap_t actionExecutors;
};



/*
 * Because this is a template enabled class,
 * the implementations have to be in the same file.
 */

template<class actionExecutor_t>
IGameCommands<actionExecutor_t>::~IGameCommands() {
	RemoveAllActionExecutors();
}

template<class actionExecutor_t>
void IGameCommands<actionExecutor_t>::AddActionExecutor(actionExecutor_t* executor) {

	const std::string commandLower = StringToLower(executor->GetCommand());
	const typename actionExecutorsMap_t::const_iterator aei
			= actionExecutors.find(commandLower);

	if (aei != actionExecutors.end()) {
		throw std::logic_error("Tried to register a duplicate action-executor for command: " + commandLower);
	} else {
		actionExecutors[commandLower] = executor;
		wordCompletion->AddWord("/" + commandLower + " ", true, false, false);
	}
}

template<class actionExecutor_t>
void IGameCommands<actionExecutor_t>::RemoveActionExecutor(const std::string& command) {

	const std::string commandLower = StringToLower(command);
	const typename actionExecutorsMap_t::iterator aei
			= actionExecutors.find(commandLower);

	if (aei != actionExecutors.end()) {
		// an executor for this command is registered
		// -> remove and delete
		actionExecutor_t* executor = aei->second;
		actionExecutors.erase(aei);
		wordCompletion->RemoveWord("/" + commandLower + " ");
		delete executor;
	}
}

template<class actionExecutor_t>
void IGameCommands<actionExecutor_t>::RemoveAllActionExecutors() {

	while (!actionExecutors.empty()) {
		RemoveActionExecutor(actionExecutors.begin()->first);
	}
}

template<class actionExecutor_t>
const actionExecutor_t* IGameCommands<actionExecutor_t>::GetActionExecutor(const std::string& command) const {

	const actionExecutor_t* executor = NULL;

	const std::string commandLower = StringToLower(command);
	const typename actionExecutorsMap_t::const_iterator aei
			= actionExecutors.find(commandLower);

	if (aei != actionExecutors.end()) {
		// an executor for this command is registered
		executor = aei->second;
	}

	return executor;
}


#endif // I_GAME_COMMANDS_H
