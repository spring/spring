/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SYNCED_GAME_COMMANDS_H
#define SYNCED_GAME_COMMANDS_H

#include <map>
#include <string>

class ISyncedActionExecutor;


// XXX Maybe template-ify this class, to reduce redundantness shared with UnsyncedGameCommands
class SyncedGameCommands
{
	SyncedGameCommands() {}
	~SyncedGameCommands();

public:
	/**
	 * This function initialized a singleton instance,
	 * if not yet done by a call to GetInstance()
	 */
	static void CreateInstance();
	static SyncedGameCommands* GetInstance() { return singleton; }
	static void DestroyInstance();

	/**
	 * Registers the default action-executors for synced chat commands.
	 * These are all the ones that are not logically tied to a specific
	 * part/sub-module of the engine (for example rendering related switches).
	 * @see AddActionExecutor
	 */
	void AddDefaultActionExecutors();

	/**
	 * Registers a new action-executor for an synced chat command.
	 * @param executor has to be new'ed, will be delete'ed internally.
	 * @see RemoveActionExecutor
	 */
	void AddActionExecutor(ISyncedActionExecutor* executor);
	/**
	 * Deregisters an action-executor for an synced chat command.
	 * The action-executor corresponding to the given command
	 * (case-insensitive) will be removed and delete'ed internally.
	 * @param command used to lookup the action-executor to be removed.
	 * @see AddActionExecutor
	 */
	void RemoveActionExecutor(const std::string& command);
	/**
	 * Deregisters all currently registered action-executor for synced chat
	 * commands.
	 * @see RemoveActionExecutor
	 */
	void RemoveAllActionExecutors();

	/**
	 * Returns the action-executor for the given command (case-insensitive).
	 * @param command used to lookup the action-executor to be removed.
	 * @return the action-executor for the given command, or NULL, if none is
	 *   registered.
	 */
	const ISyncedActionExecutor* GetActionExecutor(const std::string& command) const;

private:
	static SyncedGameCommands* singleton;

	// XXX maybe use a hash_map here, for faster lookup
	std::map<std::string, ISyncedActionExecutor*> actionExecutors;
};

#define syncedGameCommands SyncedGameCommands::GetInstance()

#endif // SYNCED_GAME_COMMANDS_H
