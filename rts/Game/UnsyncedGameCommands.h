/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNSYNCED_GAME_COMMANDS_H
#define UNSYNCED_GAME_COMMANDS_H

#include <string>

#include "IGameCommands.h"

class IUnsyncedActionExecutor;


class UnsyncedGameCommands : public IGameCommands<IUnsyncedActionExecutor>
{
	UnsyncedGameCommands() {}

public:
	/**
	 * This function initialized a singleton instance,
	 * if not yet done by a call to GetInstance()
	 */
	static void CreateInstance();
	static UnsyncedGameCommands* GetInstance() { return singleton; }
	static void DestroyInstance();

	void AddDefaultActionExecutors();

private:
	static UnsyncedGameCommands* singleton;
};

#define unsyncedGameCommands UnsyncedGameCommands::GetInstance()

#endif // UNSYNCED_GAME_COMMANDS_H
