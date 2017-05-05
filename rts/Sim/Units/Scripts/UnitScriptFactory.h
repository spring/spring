/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNITSCRIPTFACTORY_H
#define UNITSCRIPTFACTORY_H

struct UnitDef;

struct lua_State;
class CCobFile;

class CUnit;
class CUnitScript;

class CUnitScriptFactory
{
public:
	static void InitStatic();

	static CUnitScript* CreateScript(CUnit* unit, const UnitDef* udef);

	static CUnitScript* CreateCOBScript(CUnit* unit, CCobFile* F);
	static CUnitScript* CreateLuaScript(CUnit* unit, lua_State* L);
};

#endif

