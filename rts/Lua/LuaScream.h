/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_SCREAM_H
#define LUA_SCREAM_H

struct lua_State;


/**
 * This creates a userdata object, which calls a special user-defined function,
 * when the object gets destroyed (=> garbage collector).
 * You use it like this (Lua):
 * <code>
 *   local myScream = Script.CreateScream()
 *   myScream.func  = function() Spring.Echo("AHHHHHH") end
 * </code>
 */
class LuaScream {
	public:
		static bool PushEntries(lua_State* L);

	private: // metatable methods
		static bool CreateMetatable(lua_State* L);
		static int meta_gc(lua_State* L);
		static int meta_index(lua_State* L);
		static int meta_newindex(lua_State* L);

	private: // call-outs
		static int CreateScream(lua_State* L);
};


#endif /* LUA_SCREAM_H */
