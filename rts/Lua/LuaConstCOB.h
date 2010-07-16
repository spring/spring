/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* exports the #defines from CobDefines.h to Lua */

#ifndef LUACONSTCOB_H
#define LUACONSTCOB_H

struct lua_State;

class LuaConstCOB
{
	public:
		static bool PushEntries(lua_State* L);
};


class LuaConstSFX
{
	public:
		static bool PushEntries(lua_State* L);
};

#endif
