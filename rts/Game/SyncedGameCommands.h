/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SYNCED_GAME_COMMANDS_H
#define SYNCED_GAME_COMMANDS_H

#include <map>
#include <string>

#include "IGameCommands.h"

class ISyncedActionExecutor;


class SyncedGameCommands : public IGameCommands<ISyncedActionExecutor>
{
	SyncedGameCommands() {}

public:
	/**
	 * This function initialized a singleton instance,
	 * if not yet done by a call to GetInstance()
	 */
	static void CreateInstance();
	static SyncedGameCommands* GetInstance() { return singleton; }
	static void DestroyInstance();

	void AddDefaultActionExecutors();

private:
	static SyncedGameCommands* singleton;
};

#define syncedGameCommands SyncedGameCommands::GetInstance()

#endif // SYNCED_GAME_COMMANDS_H
