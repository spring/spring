/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_GAME_COMMANDS_H
#define I_GAME_COMMANDS_H

#include "System/StringUtil.h"
#include "System/UnorderedMap.hpp"

#include <array>
#include <string>


template<class TActionExecutor>
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
	void AddActionExecutor(TActionExecutor* executor) {
		const std::string& commandLower = StringToLower(executor->GetCommand());

		// prevent registering a duplicate action-executor for command
		if (actionExecutors.find(commandLower) != actionExecutors.end())
			return;

		actionExecutors[commandLower] = executor;
	}

	/**
	 * Returns the action-executor for the given command (case-insensitive).
	 * @param command used to lookup the action-executor to be removed.
	 * @return the action-executor for the given command, or NULL, if none is
	 *   registered.
	 */
	const TActionExecutor* GetActionExecutor(const std::string& command) const {
		const auto aei = actionExecutors.find(StringToLower(command));

		if (aei == actionExecutors.end())
			return nullptr;

		return aei->second;
	}


	const spring::unsynced_map<std::string, TActionExecutor*>& GetActionExecutors() const { return actionExecutors; }
	const std::vector< std::pair<std::string, TActionExecutor*> >& GetSortedActionExecutors() {
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
	/**
	 * Deregisters all currently registered action-executor for chat commands.
	 * @see RemoveActionExecutor
	 */
	void RemoveAllActionExecutors() {
		for (const auto& pair: actionExecutors) {
			pair.second->~TActionExecutor();
		}

		actionExecutors.clear();
		sortedExecutors.clear();
	}

protected:
	template<typename T, typename... A> T* AllocActionExecutor(A&&... a) {
		constexpr size_t size = sizeof(T);

		if ((actionExecMemIndex + size) > actionExecutorMem.size()) {
			assert(false);
			return nullptr;
		}

		return new (&actionExecutorMem[(actionExecMemIndex += size) - size]) T(std::forward<A>(a)...);
	}

protected:
	// currently registered lower-case commands with their respective action-executors
	spring::unsynced_map<std::string, TActionExecutor*> actionExecutors;
	std::vector< std::pair<std::string, TActionExecutor*> > sortedExecutors;

	std::array<uint8_t, 16384> actionExecutorMem;

	size_t actionExecMemIndex = 0;
	// size_t numActionExecutors = 0;
};

#endif // I_GAME_COMMANDS_H

