/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SYNCED_GAME_COMMANDS_H
#define SYNCED_GAME_COMMANDS_H

#include "IGameCommands.h"

class ISyncedActionExecutor;


class SyncedGameCommands : public IGameCommands<ISyncedActionExecutor>
{
public:
	static SyncedGameCommands*& GetInstance() {
		static SyncedGameCommands* singleton = nullptr;
		return singleton;
	}

	static void CreateInstance();
	static void DestroyInstance(bool reload);

	void AddDefaultActionExecutors() override;
};

#define syncedGameCommands SyncedGameCommands::GetInstance()

#endif // SYNCED_GAME_COMMANDS_H
