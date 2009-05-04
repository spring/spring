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
	static int PieceEmitSfx(lua_State* L);       // TODO: better names?
	static int PieceAttachUnit(lua_State* L);
	static int PieceDropUnit(lua_State* L);
	static int PieceExplode(lua_State* L);
	static int PieceShowFlare(lua_State* L);

	// Lua COB replacement animation support funcs
	static int PieceSpin(lua_State* L);
	static int PieceStopSpin(lua_State* L);
	static int PieceTurn(lua_State* L);
	static int PieceMove(lua_State* L);
};

#endif
