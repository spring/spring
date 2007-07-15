#include "StdAfx.h"
// LuaRules.cpp: implementation of the CLuaRules class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaHandle.h"
#include <string>

#include "LuaCallInHandler.h"
#include "LuaHashString.h"
#include "LuaOpenGL.h"
#include "LuaBitOps.h"
#include "Game/UI/MiniMap.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Sim/Units/Unit.h"
#include "System/LogOutput.h"
#include "System/FileSystem/FileHandler.h"

#include "LuaInclude.h"


bool CLuaHandle::devMode = false;

CLuaHandle* CLuaHandle::activeHandle = NULL;
bool CLuaHandle::activeFullRead    = false;
int CLuaHandle::activeReadAllyTeam = CLuaHandle::NoAccessTeam;


/******************************************************************************/
/******************************************************************************/

CLuaHandle::CLuaHandle(const string& _name, int _order,
                       bool _userMode, LuaCobCallback callback)
: name       (_name),
  order      (_order),
  userMode   (_userMode),
  cobCallback(callback),
  killMe     (false),
  synced     (false)
{
	L = lua_open();
}


CLuaHandle::~CLuaHandle()
{
	luaCallIns.RemoveHandle(this);

	// free the lua state
	if (L != NULL) {
		lua_close(L);
		L = NULL;
	}

	if (this == activeHandle) {
		activeHandle = NULL;
	}
}


/******************************************************************************/
/******************************************************************************/

int CLuaHandle::KillActiveHandle(lua_State* L)
{
	if (activeHandle) {
		const int args = lua_gettop(L);
		if ((args >= 1) && lua_isstring(L, 1)) {
			activeHandle->killMsg = lua_tostring(L, 1);
		}
		activeHandle->killMe = true;
	}
	return 0;
}


/******************************************************************************/

bool CLuaHandle::AddEntriesToTable(lua_State* L, const char* name,
                                   bool (*entriesFunc)(lua_State*))
{
	const int top = lua_gettop(L);
	lua_pushstring(L, name);
	lua_rawget(L, -2);
	if (lua_istable(L, -1)) {
		bool success = entriesFunc(L);
		lua_settop(L, top);
		return success;
	}

	// make a new table
	lua_pop(L, 1);
	lua_pushstring(L, name);
	lua_newtable(L);
	if (!entriesFunc(L)) {
		lua_settop(L, top);
		return false;
	}
	lua_rawset(L, -3);

	lua_settop(L, top);
	return true;
}


bool CLuaHandle::LoadCode(const string& code, const string& debug)
{
	lua_settop(L, 0);

	int error;
	error = luaL_loadbuffer(L, code.c_str(), code.size(), debug.c_str());
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n",
		                error, debug.c_str(), lua_tostring(L, -1));
		lua_settop(L, 0);
		return false;
	}

	CLuaHandle* orig = activeHandle;
	SetActiveHandle();
	error = lua_pcall(L, 0, 0, 0);
	SetActiveHandle(orig);

	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n",
		                error, debug.c_str(), lua_tostring(L, -1));
		lua_settop(L, 0);
		return false;
	}

	return true;
}


/******************************************************************************/
/******************************************************************************/

bool CLuaHandle::RunCallIn(const LuaHashString& hs, int inArgs, int outArgs)
{
	CLuaHandle* orig = activeHandle;
	SetActiveHandle();
	const int error = lua_pcall(L, inArgs, outArgs, 0);
	SetActiveHandle(orig);

	if (error != 0) {
		logOutput.Print("%s: error = %i, %s, %s\n", name.c_str(), error,
		                hs.GetString().c_str(), lua_tostring(L, -1));
		lua_settop(L, 0);
		return false;
	}
	return true;
}


/******************************************************************************/

void CLuaHandle::Shutdown()
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("Shutdown");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return; // the call is not defined
	}

	// call the routine
	RunCallIn(cmdStr, 0, 0);
	return;
}


void CLuaHandle::GameOver()
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("GameOver");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return; // the call is not defined
	}

	// call the routine
	RunCallIn(cmdStr, 0, 0);
	return;
}


void CLuaHandle::TeamDied(int teamID)
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("TeamDied");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return; // the call is not defined
	}

	lua_pushnumber(L, teamID);

	// call the routine
	RunCallIn(cmdStr, 1, 0);
	return;
}


/******************************************************************************/

void CLuaHandle::UnitCreated(const CUnit* unit, const CUnit* builder)
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("UnitCreated");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	if (builder != NULL) {
		lua_pushnumber(L, builder->id);
	}

	// call the routine
	RunCallIn(cmdStr, (builder != NULL) ? 4 : 3, 0);
	return;
}


void CLuaHandle::UnitFinished(const CUnit* unit)
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("UnitFinished");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);

	// call the routine
	RunCallIn(cmdStr, 3, 0);
	return;
}


void CLuaHandle::UnitFromFactory(const CUnit* unit,
                                 const CUnit* factory, bool userOrders)
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("UnitFromFactory");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, factory->id);
	lua_pushnumber(L, factory->unitDef->id);
	lua_pushboolean(L, userOrders);

	// call the routine
	RunCallIn(cmdStr, 6, 0);
	return;
}


void CLuaHandle::UnitDestroyed(const CUnit* unit, const CUnit* attacker)
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("UnitDestroyed");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return; // the call is not defined
	}

	int argCount = 3;
	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	if (fullRead && (attacker != NULL)) {
		lua_pushnumber(L, attacker->id);
		lua_pushnumber(L, attacker->unitDef->id);
		lua_pushnumber(L, attacker->team);
		argCount += 3;
	}

	// call the routine
	RunCallIn(cmdStr, argCount, 0);
	return;
}


void CLuaHandle::UnitTaken(const CUnit* unit, int newTeam)
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("UnitTaken");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, newTeam);

	// call the routine
	RunCallIn(cmdStr, 4, 0);
	return;
}


void CLuaHandle::UnitGiven(const CUnit* unit, int oldTeam)
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("UnitGiven");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, oldTeam);

	// call the routine
	RunCallIn(cmdStr, 4, 0);
	return;
}


void CLuaHandle::UnitIdle(const CUnit* unit)
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("UnitIdle");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);

	// call the routine
	RunCallIn(cmdStr, 3, 0);
	return;
}


void CLuaHandle::UnitDamaged(const CUnit* unit, const CUnit* attacker,
                             float damage, bool paralyzer)
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("UnitDamaged");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return; // the call is not defined
	}

	int argCount = 5;
	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, damage);
	lua_pushboolean(L, paralyzer);
	if (fullRead && (attacker != NULL)) {
		lua_pushnumber(L, attacker->id);
		lua_pushnumber(L, attacker->unitDef->id);
		lua_pushnumber(L, attacker->team);
		argCount += 3;
	}

	// call the routine
	RunCallIn(cmdStr, argCount, 0);
	return;
}


/******************************************************************************/

void CLuaHandle::UnitSeismicPing(const CUnit* unit, int allyTeam,
                                 const float3& pos, float strength)
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("UnitSeismicPing");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return; // the call is not defined
	}

	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
	lua_pushnumber(L, strength);
	if (fullRead) {
		lua_pushnumber(L, allyTeam);
		lua_pushnumber(L, unit->id);
		lua_pushnumber(L, unit->unitDef->id);
	}

	// call the routine
	RunCallIn(cmdStr, fullRead ? 7 : 4, 0);
	return;
}


/******************************************************************************/

void CLuaHandle::LosCallIn(const LuaHashString& hs,
                           const CUnit* unit, int allyTeam)
{
	lua_settop(L, 0);
	if (!hs.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->team);
	if (fullRead) {
		lua_pushnumber(L, allyTeam);
		lua_pushnumber(L, unit->unitDef->id);
	}

	// call the routine
	RunCallIn(hs, fullRead ? 4 : 2, 0);
	return;
}


void CLuaHandle::UnitEnteredRadar(const CUnit* unit, int allyTeam)
{
	static const LuaHashString hs(__FUNCTION__);
	LosCallIn(hs, unit, allyTeam);
}


void CLuaHandle::UnitEnteredLos(const CUnit* unit, int allyTeam)
{
	static const LuaHashString hs(__FUNCTION__);
	LosCallIn(hs, unit, allyTeam);
}


void CLuaHandle::UnitLeftRadar(const CUnit* unit, int allyTeam)
{
	static const LuaHashString hs(__FUNCTION__);
	LosCallIn(hs, unit, allyTeam);
}


void CLuaHandle::UnitLeftLos(const CUnit* unit, int allyTeam)
{
	static const LuaHashString hs(__FUNCTION__);
	LosCallIn(hs, unit, allyTeam);
}


/******************************************************************************/

void CLuaHandle::UnitLoaded(const CUnit* unit, const CUnit* transport)
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("UnitLoaded");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, transport->id);
	lua_pushnumber(L, transport->team);

	// call the routine
	RunCallIn(cmdStr, 5, 0);
	return;
}


void CLuaHandle::UnitUnloaded(const CUnit* unit, const CUnit* transport)
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("UnitUnloaded");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, transport->id);
	lua_pushnumber(L, transport->team);

	// call the routine
	RunCallIn(cmdStr, 5, 0);
	return;
}


/******************************************************************************/

void CLuaHandle::FeatureCreated(const CFeature* feature)
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("FeatureCreated");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return; // the call is not defined
	}

	lua_pushnumber(L, feature->id);
	lua_pushnumber(L, feature->allyteam);

	// call the routine
	RunCallIn(cmdStr, 2, 0);
	return;
}


void CLuaHandle::FeatureDestroyed(const CFeature* feature)
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("FeatureDestroyed");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return; // the call is not defined
	}

	lua_pushnumber(L, feature->id);
	lua_pushnumber(L, feature->allyteam);

	// call the routine
	RunCallIn(cmdStr, 2, 0);
	return;
}


/******************************************************************************/

bool CLuaHandle::Explosion(int weaponID, const float3& pos, const CUnit* owner)
{
	if ((weaponID >= (int)watchWeapons.size()) || (weaponID < 0)) {
		return false;
	}
	if (!watchWeapons[weaponID]) {
		return false;
	}

	lua_settop(L, 0);
	static const LuaHashString cmdStr("Explosion");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return false; // the call is not defined
	}

	lua_pushnumber(L, weaponID);
	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
	if (owner != NULL) {
		lua_pushnumber(L, owner->id);
	}

	// call the routine
	RunCallIn(cmdStr, (owner == NULL) ? 4 : 5, 1);

	// get the results
	const int args = lua_gettop(L);
	if ((args != 1) || !lua_isboolean(L, -1)) {
		logOutput.Print("%s() bad return value (%i)\n",
		                cmdStr.GetString().c_str(), args);
		lua_settop(L, 0);
		return false;
	}

	return !!lua_toboolean(L, -1);
}


/******************************************************************************/

inline bool CLuaHandle::LoadDrawCallIn(const LuaHashString& hs)
{
	// LuaUI keeps these call-ins in the Global table,
	// the synced handles keep them in the Registry table
	if (userMode) {
		return hs.GetGlobalFunc(L);
	} else {
		return hs.GetRegistryFunc(L);
	}
}


void CLuaHandle::Update()
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("Update");
	if (!LoadDrawCallIn(cmdStr)) {
		lua_settop(L, 0);
		return;
	}

	synced = false;

	// call the routine
	RunCallIn(cmdStr, 0, 0);

	synced = !userMode;

	return;
}


void CLuaHandle::DrawWorld()
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("DrawWorld");
	if (!LoadDrawCallIn(cmdStr)) {
		lua_settop(L, 0);
		return;
	}

	synced = false;

	// call the routine
	RunCallIn(cmdStr, 0, 0);

	synced = !userMode;

	return;
}


void CLuaHandle::DrawWorldPreUnit()
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("DrawWorldPreUnit");
	if (!LoadDrawCallIn(cmdStr)) {
		lua_settop(L, 0);
		return;
	}

	synced = false;

	// call the routine
	RunCallIn(cmdStr, 0, 0);

	synced = !userMode;

	return;
}


void CLuaHandle::DrawWorldShadow()
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("DrawWorldShadow");
	if (!LoadDrawCallIn(cmdStr)) {
		lua_settop(L, 0);
		return;
	}

	synced = false;

	// call the routine
	RunCallIn(cmdStr, 0, 0);

	synced = !userMode;

	return;
}


void CLuaHandle::DrawWorldReflection()
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("DrawWorldReflection");
	if (!LoadDrawCallIn(cmdStr)) {
		lua_settop(L, 0);
		return;
	}

	synced = false;

	// call the routine
	RunCallIn(cmdStr, 0, 0);

	synced = !userMode;

	return;
}


void CLuaHandle::DrawWorldRefraction()
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("DrawWorldRefraction");
	if (!LoadDrawCallIn(cmdStr)) {
		lua_settop(L, 0);
		return;
	}

	synced = false;

	// call the routine
	RunCallIn(cmdStr, 0, 0);

	synced = !userMode;

	return;
}


void CLuaHandle::DrawScreen()
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("DrawScreen");
	if (!LoadDrawCallIn(cmdStr)) {
		lua_settop(L, 0);
		return;
	}
	
	lua_pushnumber(L, gu->viewSizeX);
	lua_pushnumber(L, gu->viewSizeY);

	synced = false;

	// call the routine
	RunCallIn(cmdStr, 2, 0);

	synced = !userMode;

	return;
}


void CLuaHandle::DrawScreenEffects()
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("DrawScreenEffects");
	if (!LoadDrawCallIn(cmdStr)) {
		lua_settop(L, 0);
		return;
	}
	
	lua_pushnumber(L, gu->viewSizeX);
	lua_pushnumber(L, gu->viewSizeY);

	synced = false;

	// call the routine
	RunCallIn(cmdStr, 2, 0);

	synced = !userMode;

	return;
}


void CLuaHandle::DrawInMiniMap()
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("DrawInMiniMap");
	if (!LoadDrawCallIn(cmdStr)) {
		lua_settop(L, 0);
		return;
	}

	const int xSize = minimap->GetSizeX();
	const int ySize = minimap->GetSizeY();
	
	if ((xSize < 1) || (ySize < 1)) {
		lua_settop(L, 0);
		return;
	}
	
	lua_pushnumber(L, xSize);
	lua_pushnumber(L, ySize);

	synced = false;

	// call the routine
	RunCallIn(cmdStr, 2, 0);

	synced = !userMode;

	return;
}


/******************************************************************************/

bool CLuaHandle::AddBasicCalls()
{
	HSTR_PUSH(L, "Script");
	lua_newtable(L); {
		HSTR_PUSH_CFUNC(L, "Kill",            KillActiveHandle);
		HSTR_PUSH_CFUNC(L, "GetName",         CallOutGetName);
		HSTR_PUSH_CFUNC(L, "GetSynced",       CallOutGetSynced);
		HSTR_PUSH_CFUNC(L, "GetFullCtrl",     CallOutGetFullCtrl);
		HSTR_PUSH_CFUNC(L, "GetFullRead",     CallOutGetFullRead);
		HSTR_PUSH_CFUNC(L, "GetCtrlTeam",     CallOutGetCtrlTeam);
		HSTR_PUSH_CFUNC(L, "GetReadTeam",     CallOutGetReadTeam);
		HSTR_PUSH_CFUNC(L, "GetReadAllyTeam", CallOutGetReadAllyTeam);
		HSTR_PUSH_CFUNC(L, "GetSelectTeam",   CallOutGetSelectTeam);
		HSTR_PUSH_CFUNC(L, "GetGlobal",       CallOutGetGlobal);
		HSTR_PUSH_CFUNC(L, "GetRegistry",     CallOutGetRegistry);
		// special team constants
		HSTR_PUSH_NUMBER(L, "NO_ACCESS_TEAM",  NoAccessTeam);	
		HSTR_PUSH_NUMBER(L, "ALL_ACCESS_TEAM", AllAccessTeam);	
	}
	lua_rawset(L, -3);

	// boolean operations
	lua_getglobal(L, "math");
	LuaBitOps::PushEntries(L);
	lua_pop(L, 1);
	return true;	
}


int CLuaHandle::CallOutGetName(lua_State* L)
{
	lua_pushstring(L, activeHandle->name.c_str());
	return 1;
}


int CLuaHandle::CallOutGetSynced(lua_State* L)
{
	lua_pushboolean(L, activeHandle->synced);
	return 1;
}


int CLuaHandle::CallOutGetFullCtrl(lua_State* L)
{
	lua_pushboolean(L, activeHandle->fullCtrl);
	return 1;
}


int CLuaHandle::CallOutGetFullRead(lua_State* L)
{
	lua_pushboolean(L, activeHandle->fullRead);
	return 1;
}


int CLuaHandle::CallOutGetCtrlTeam(lua_State* L)
{
	lua_pushnumber(L, activeHandle->ctrlTeam);
	return 1;
}


int CLuaHandle::CallOutGetReadTeam(lua_State* L)
{
	lua_pushnumber(L, activeHandle->readTeam);
	return 1;
}


int CLuaHandle::CallOutGetReadAllyTeam(lua_State* L)
{
	lua_pushnumber(L, activeHandle->readAllyTeam);
	return 1;
}


int CLuaHandle::CallOutGetSelectTeam(lua_State* L)
{
	lua_pushnumber(L, activeHandle->selectTeam);
	return 1;
}


int CLuaHandle::CallOutGetGlobal(lua_State* L)
{
	if (devMode) {
		lua_pushvalue(L, LUA_GLOBALSINDEX);
		return 1;
	}
	return 0;
}


int CLuaHandle::CallOutGetRegistry(lua_State* L)
{
	if (devMode) {
		lua_pushvalue(L, LUA_REGISTRYINDEX);
		return 1;
	}
	return 0;
}


int CLuaHandle::CallOutSyncedUpdateCallIn(lua_State* L)
{
	const int args = lua_gettop(L);
	if ((args != 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to UpdateCallIn()");
	}
	const string name = lua_tostring(L, 1);
	activeHandle->SyncedUpdateCallIn(name);
	return 0;
}


int CLuaHandle::CallOutUnsyncedUpdateCallIn(lua_State* L)
{
	const int args = lua_gettop(L);
	if ((args != 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to UpdateCallIn()");
	}
	const string name = lua_tostring(L, 1);
	activeHandle->UnsyncedUpdateCallIn(name);
	return 0;
}


/******************************************************************************/
/******************************************************************************/
