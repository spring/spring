/* Author: Tobi Vollebregt */

#ifndef LUAUNITSCRIPT_H
#define LUAUNITSCRIPT_H

#include "UnitScript.h"


struct lua_State;


class CLuaUnitScript : public CUnitScript
{
public:

public:
	static bool PushEntries(lua_State* L);

	// Lua COB replacement support funcs (+SpawnCEG, PlaySoundFile, etc.)
	static int GetUnitCOBValue(lua_State* L);
	static int SetUnitCOBValue(lua_State* L);
	static int SetPieceVisibility(lua_State* L);
	static int EmitSfx(lua_State* L);       // TODO: better names?
	static int AttachUnit(lua_State* L);
	static int DropUnit(lua_State* L);
	static int Explode(lua_State* L);
	static int ShowFlare(lua_State* L);

	// Lua COB replacement animation support funcs
	static int Spin(lua_State* L);
	static int StopSpin(lua_State* L);
	static int Turn(lua_State* L);
	static int Move(lua_State* L);
};

#endif
