/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_UI_COMMAND_H
#define LUA_UI_COMMAND_H

struct lua_State;

/* LuaUICommand defines functions to read and mofiy the engine UI commands (strings written in the console starting with "/", e.g. /luaui reload). */
class LuaUICommand {
	public:
		static bool PushEntries(lua_State* L);
		static int GetUICommands(lua_State* L);
		// TODO: Add these commands in the future
		// 		static int RegisterUICommand(lua_State* L);
		// 		static int DeregisterUICommand(lua_State* L);
};

#endif /* LUA_UI_COMMAND_H */
