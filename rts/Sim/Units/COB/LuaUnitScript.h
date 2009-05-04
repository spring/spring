/* Author: Tobi Vollebregt */

#ifndef LUAUNITSCRIPT_H
#define LUAUNITSCRIPT_H

#include "UnitScript.h"


struct lua_State;


class CLuaUnitScript : public CUnitScript
{
private:
	// contrary to COB the list of functions may differ per unit,
	// so the COBFN_* -> function mapping can differ per unit too.
	std::vector<int> scriptIndex;

protected:
	virtual int RealCall(int functionId, std::vector<int> &args, CBCobThreadFinish cb, void *p1, void *p2);
	virtual void ShowScriptError(const std::string& msg);
	virtual void ShowScriptWarning(const std::string& msg);

public:
	CLuaUnitScript(CUnit* _unit);
	~CLuaUnitScript();

	virtual int GetFunctionId(const std::string& fname) const;

public:
	static bool PushEntries(lua_State* L);

private:
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
	static int IsInTurn(lua_State* L);
	static int IsInMove(lua_State* L);
	static int IsInSpin(lua_State* L);
};

#endif
