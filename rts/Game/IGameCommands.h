/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_GAME_COMMANDS_H
#define I_GAME_COMMANDS_H

#include "System/StringUtil.h"
#include "System/UnorderedMap.hpp"

// These two are required for the destructors
#include "SyncedActionExecutor.h"
#include "UnsyncedActionExecutor.h"

#include <string>
#include <stdexcept>


template<class actionExecutor_t>
class IGameCommands
{
protected:
	IGameCommands() { actionExecutors.reserve(64); }
	virtual ~IGameCommands() { RemoveAllActionExecutors(); }

public:
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
	void RemoveAllActionExecutors() {
		for (const auto& pair: actionExecutors) {
			delete pair.second;
		}

		actionExecutors.clear();
		sortedExecutors.clear();
	}

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
	const spring::unsynced_map<std::string, actionExecutor_t*>& GetActionExecutors() const { return actionExecutors; }
	const std::vector< std::pair<std::string, actionExecutor_t*> >& GetSortedActionExecutors() {
		using P = typename decltype(sortedExecutors)::value_type;

		// no need for caching, very rarely called
		sortedExecutors.clear();
		sortedExecutors.reserve(actionExecutors.size());

		for (const auto& pair: actionExecutors) {
			sortedExecutors.emplace_back(pair);
		}

		std::sort(sortedExecutors.begin(), sortedExecutors.end(), [](const P& a, const P& b) { return (a.first < b.first); });
		return sortedExecutors;
	}

private:
	spring::unsynced_map<std::string, actionExecutor_t*> actionExecutors;
	std::vector< std::pair<std::string, actionExecutor_t*> > sortedExecutors;
};



/*
 * Because this is a template enabled class,
 * the implementations have to be in the same file.
 */

template<class actionExecutor_t>
void IGameCommands<actionExecutor_t>::AddActionExecutor(actionExecutor_t* executor)
{
	const std::string commandLower = StringToLower(executor->GetCommand());

	// prevent registering a duplicate action-executor for command
	if (actionExecutors.find(commandLower) != actionExecutors.end())
		return;

	actionExecutors[commandLower] = executor;
}

template<class actionExecutor_t>
void IGameCommands<actionExecutor_t>::RemoveActionExecutor(const std::string& command)
{
	const auto aei = actionExecutors.find(StringToLower(command));

	if (aei == actionExecutors.end())
		return;

	// an executor for this command is registered; remove and delete
	actionExecutor_t* executor = aei->second;
	actionExecutors.erase(aei);
	delete executor;
}


template<class actionExecutor_t>
const actionExecutor_t* IGameCommands<actionExecutor_t>::GetActionExecutor(const std::string& command) const
{
	const auto aei = actionExecutors.find(StringToLower(command));

	if (aei == actionExecutors.end())
		return nullptr;

	// an executor for this command is registered
	return aei->second;
}


#endif // I_GAME_COMMANDS_H
