/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SYNCED_GAME_COMMANDS_H
#define SYNCED_GAME_COMMANDS_H

class CGame;

class SyncedGameCommands
{
	/// Private ctor, so the clas can not be instantiated
	SyncedGameCommands() {}
public:
	static void RegisterDefaultExecutors(CGame* game);
};

#endif // SYNCED_GAME_COMMANDS_H
