/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNSYNCED_GAME_COMMANDS_H
#define UNSYNCED_GAME_COMMANDS_H

#include "IGameCommands.h"

class IUnsyncedActionExecutor;


class UnsyncedGameCommands : public IGameCommands<IUnsyncedActionExecutor>
{
public:
	static UnsyncedGameCommands*& GetInstance() {
		static UnsyncedGameCommands* singleton = nullptr;
		return singleton;
	}

	static void CreateInstance();
	static void DestroyInstance(bool reload);

	void AddDefaultActionExecutors() override;
};

#define unsyncedGameCommands UnsyncedGameCommands::GetInstance()

#endif // UNSYNCED_GAME_COMMANDS_H
