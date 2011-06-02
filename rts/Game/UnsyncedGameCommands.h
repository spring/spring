/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNSYNCED_GAME_COMMANDS_H
#define UNSYNCED_GAME_COMMANDS_H

class CGame;

class UnsyncedGameCommands
{
	/// Private ctor, so the clas can not be instantiated
	UnsyncedGameCommands() {}
public:
	static void RegisterDefaultExecutors(CGame* game);
};

#endif // UNSYNCED_GAME_COMMANDS_H
