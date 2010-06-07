/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include <string>
#include <SDL_keysym.h>
#include <SDL_mouse.h>
#include <SDL_timer.h>

#include "mmgr.h"

#include "LuaHandle.h"

#include "Game/UI/LuaUI.h"
#include "LuaGaia.h"
#include "LuaRules.h"

#include "LuaCallInCheck.h"
#include "LuaHashString.h"
#include "LuaOpenGL.h"
#include "LuaBitOps.h"
#include "LuaUtils.h"
#include "LuaZip.h"
#include "Game/PlayerHandler.h"
#include "Game/UI/KeyCodes.h"
#include "Game/UI/KeySet.h"
#include "Game/UI/KeyBindings.h"
#include "Game/UI/MiniMap.h"
#include "Rendering/InMapDraw.h"
#include "Rendering/GlobalRendering.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Weapons/Weapon.h"
#include "System/BaseNetProtocol.h"
#include "System/EventHandler.h"
#include "System/LogOutput.h"
#include "System/SpringApp.h"
#include "System/FileSystem/FileHandler.h"

#include "LuaInclude.h"

extern boost::uint8_t *keys;

bool CLuaHandle::devMode = false;
bool CLuaHandle::modUICtrl = true;

CLuaHandle* CLuaHandle::activeHandle = NULL;
bool CLuaHandle::activeFullRead    = false;
int CLuaHandle::activeReadAllyTeam = CEventClient::NoAccessTeam;


/******************************************************************************/
/******************************************************************************/

CLuaHandle::CLuaHandle(const string& _name, int _order, bool _userMode)
: CEventClient(_name, _order, false), // FIXME
  userMode   (_userMode),
  killMe     (false),
  synced     (false),
#ifdef DEBUG
  printTracebacks(true),
#else
  printTracebacks(false),
#endif
  callinErrors(0)
{
	L = lua_open();
	luaopen_debug(L);
}


CLuaHandle::~CLuaHandle()
{
	eventHandler.RemoveClient(this);

	// free the lua state
	KillLua();

	if (this == activeHandle) {
		activeHandle = NULL;
	}
}


void CLuaHandle::KillLua()
{
	if (L != NULL) {
		CLuaHandle* orig = activeHandle;
		SetActiveHandle();
		lua_close(L);
		SetActiveHandle(orig);
	}
	L = NULL;
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

#if defined(__SUPPORT_SNAN__) && !defined(USE_GML)
	// do not signal floating point exceptions in user Lua code
	feclearexcept(streflop::FPU_Exceptions(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW));
#endif

	int error;
	error = luaL_loadbuffer(L, code.c_str(), code.size(), debug.c_str());
	if (error != 0) {
		logOutput.Print("Lua LoadCode loadbuffer error = %i, %s, %s\n",
		                error, debug.c_str(), lua_tostring(L, -1));
		lua_pop(L, 1);
#if defined(__SUPPORT_SNAN__) && !defined(USE_GML)
		feraiseexcept(streflop::FPU_Exceptions(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW));
#endif
		return false;
	}

	CLuaHandle* orig = activeHandle;
	SetActiveHandle();
	error = lua_pcall(L, 0, 0, 0);
	SetActiveHandle(orig);

#if defined(__SUPPORT_SNAN__) && !defined(USE_GML)
	feraiseexcept(streflop::FPU_Exceptions(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW));
#endif

	if (error != 0) {
		logOutput.Print("Lua LoadCode pcall error = %i, %s, %s\n",
		                error, debug.c_str(), lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}

	return true;
}

/******************************************************************************/
/******************************************************************************/

void CLuaHandle::CheckStack()
{
	GML_RECMUTEX_LOCK(lua); // CheckStack - avoid bogus errors due to concurrency

	const int top = lua_gettop(L);
	if (top != 0) {
		logOutput.Print("WARNING: %s stack check: top = %i\n", GetName().c_str(), top);
		lua_settop(L, 0);
	}
}


/******************************************************************************/
/******************************************************************************/

int CLuaHandle::SetupTraceback()
{
	if (!printTracebacks)
		return 0;

	return LuaUtils::PushDebugTraceback(L);
}


int CLuaHandle::RunCallInTraceback(int inArgs, int outArgs, int errfuncIndex, std::string& traceback)
{
#if defined(__SUPPORT_SNAN__) && !defined(USE_GML)
	// do not signal floating point exceptions in user Lua code
	feclearexcept(streflop::FPU_Exceptions(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW));
#endif

	CLuaHandle* orig = activeHandle;
	SetActiveHandle();
	//! limit gc just to the time the correct ActiveHandle is bound,
	//! because some object could use __gc and try to access the ActiveHandle
	//! outside of SetActiveHandle this can be an incorrect enviroment or even null -> crash
	lua_gc(L,LUA_GCRESTART,0);
	const int error = lua_pcall(L, inArgs, outArgs, errfuncIndex);
	lua_gc(L,LUA_GCSTOP,0);
	SetActiveHandle(orig);

	if (error == 0) {
		// pop the error handler
		if (errfuncIndex != 0) {
			lua_remove(L, errfuncIndex);
		}
	} else {
		traceback = lua_tostring(L, -1);
		lua_pop(L, 1);
		if (errfuncIndex != 0)
			lua_remove(L, errfuncIndex);
		// log only errors that lead to a crash
		callinErrors += (error == 2);
	}

#if defined(__SUPPORT_SNAN__) && !defined(USE_GML)
	feraiseexcept(streflop::FPU_Exceptions(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW));
#endif
	return error;
}


bool CLuaHandle::RunCallInTraceback(const LuaHashString& hs, int inArgs, int outArgs, int errfuncIndex)
{
	std::string traceback;
	const int error = RunCallInTraceback(inArgs, outArgs, errfuncIndex, traceback);

	if (error != 0) {
		logOutput.Print("%s::RunCallIn: error = %i, %s, %s\n", GetName().c_str(),
		                error, hs.GetString().c_str(), traceback.c_str());
		return false;
	}
	return true;
}


int CLuaHandle::RunCallIn(int inArgs, int outArgs, std::string& errormessage)
{
	return RunCallInTraceback(inArgs, outArgs, 0, errormessage);
}


bool CLuaHandle::RunCallIn(const LuaHashString& hs, int inArgs, int outArgs)
{
	return RunCallInTraceback(hs, inArgs, outArgs, 0);
}



inline bool CLuaHandle::RunCallInUnsynced(const LuaHashString& hs,
                                          int inArgs, int outArgs)
{
	synced = false;
	const bool retval = RunCallIn(hs, inArgs, outArgs);
	synced = !userMode;
	return retval;
}


/******************************************************************************/

void CLuaHandle::Shutdown()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);

	int errfunc = SetupTraceback();

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		// remove error handler
		if (errfunc) lua_pop(L, 1);
		return; // the call is not defined
	}

	// call the routine
	RunCallInTraceback(cmdStr, 0, 0, errfunc);
	return;
}

void CLuaHandle::Load(CArchiveBase* archive)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);

	int errfunc = SetupTraceback();

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		// remove error handler
		if (errfunc) lua_pop(L, 1);
		return; // the call is not defined
	}

	// Load gets ZipFileReader userdatum as single argument
	LuaZipFileReader::PushNew(L, "", archive);

	// call the routine
	RunCallInTraceback(cmdStr, 1, 0, errfunc);
	return;
}

void CLuaHandle::GamePreload()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);

	int errfunc = SetupTraceback();

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		// remove error handler
		if (errfunc) lua_pop(L, 1);
		return; // the call is not defined
	}

	// call the routine
	RunCallInTraceback(cmdStr, 0, 0, errfunc);
	return;
}

void CLuaHandle::GameStart()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);

	int errfunc = SetupTraceback();

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		// remove error handler
		if (errfunc) lua_pop(L, 1);
		return; // the call is not defined
	}

	// call the routine
	RunCallInTraceback(cmdStr, 0, 0, errfunc);
	return;
}

void CLuaHandle::GameOver()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);

	int errfunc = SetupTraceback();

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		// remove error handler
		if (errfunc) lua_pop(L, 1);
		return; // the call is not defined
	}

	// call the routine
	RunCallInTraceback(cmdStr, 0, 0, errfunc);
	return;
}


void CLuaHandle::TeamDied(int teamID)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);

	int errfunc = SetupTraceback();

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		// remove error handler
		if (errfunc) lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, teamID);

	// call the routine
	RunCallInTraceback(cmdStr, 1, 0, errfunc);
	return;
}


void CLuaHandle::TeamChanged(int teamID)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);

	int errfunc = SetupTraceback();

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		// remove error handler
		if (errfunc) lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, teamID);

	// call the routine
	RunCallInTraceback(cmdStr, 1, 0, errfunc);
	return;
}


void CLuaHandle::PlayerChanged(int playerID)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);

	int errfunc = SetupTraceback();

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		// remove error handler
		if (errfunc) lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, playerID);

	// call the routine
	RunCallInTraceback(cmdStr, 1, 0, errfunc);
	return;
}


void CLuaHandle::PlayerRemoved(int playerID, int reason)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);

	int errfunc = SetupTraceback();

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		// remove error handler
		if (errfunc) lua_pop(L, 1);
		return; // the call is not defined	}
	}

	lua_pushnumber(L, playerID);
	lua_pushnumber(L, reason);

	// call the routine
	RunCallInTraceback(cmdStr, 2, 0, errfunc);
	return;
}


/******************************************************************************/

inline void CLuaHandle::UnitCallIn(const LuaHashString& hs, const CUnit* unit)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 6);
	int errfunc = SetupTraceback();

	if (!hs.GetGlobalFunc(L)) {
		// remove error handler
		if (errfunc) lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);

	// call the routine
	RunCallInTraceback(hs, 3, 0, errfunc);

	return;
}


void CLuaHandle::UnitCreated(const CUnit* unit, const CUnit* builder)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 7);

	int errfunc = SetupTraceback();

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		// remove error handler
		if (errfunc) lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	if (builder != NULL) {
		lua_pushnumber(L, builder->id);
	}

	int args = (builder != NULL) ? 4 : 3;
	// call the routine
	RunCallInTraceback(cmdStr, args, 0, errfunc);
	return;
}


void CLuaHandle::UnitFinished(const CUnit* unit)
{
	static const LuaHashString cmdStr(__FUNCTION__);
	UnitCallIn(cmdStr, unit);
	return;
}


void CLuaHandle::UnitFromFactory(const CUnit* unit,
                                 const CUnit* factory, bool userOrders)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 9);

	int errfunc = SetupTraceback();

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		// remove error handler
		if (errfunc) lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, factory->id);
	lua_pushnumber(L, factory->unitDef->id);
	lua_pushboolean(L, userOrders);

	// call the routine
	RunCallInTraceback(cmdStr, 6, 0, errfunc);
	return;
}


void CLuaHandle::UnitDestroyed(const CUnit* unit, const CUnit* attacker)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 9);

	int errfunc = SetupTraceback();

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		// remove error handler
		if (errfunc) lua_pop(L, 1);
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
	RunCallInTraceback(cmdStr, argCount, 0, errfunc);
	return;
}


void CLuaHandle::UnitTaken(const CUnit* unit, int newTeam)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 7);
	int errfunc = SetupTraceback();

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		// remove error handler
		if (errfunc) lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, newTeam);

	// call the routine
	RunCallInTraceback(cmdStr, 4, 0, errfunc);
	return;
}


void CLuaHandle::UnitGiven(const CUnit* unit, int oldTeam)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 7);
	int errfunc = SetupTraceback();

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		// remove error handler
		if (errfunc) lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, oldTeam);

	// call the routine
	RunCallInTraceback(cmdStr, 4, 0, errfunc);
	return;
}


void CLuaHandle::UnitIdle(const CUnit* unit)
{
	static const LuaHashString cmdStr(__FUNCTION__);
	UnitCallIn(cmdStr, unit);
	return;
}


void CLuaHandle::UnitCommand(const CUnit* unit, const Command& command)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 11);

	int errfunc = SetupTraceback();

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		// remove error handler
		if (errfunc) lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);

	lua_pushnumber(L, command.id);
	lua_pushnumber(L, command.options);

	const vector<float> params = command.params;
	lua_createtable(L, params.size(), 0);
	for (unsigned int i = 0; i < params.size(); i++) {
		lua_pushnumber(L, i + 1);
		lua_pushnumber(L, params[i]);
		lua_rawset(L, -3);
	}

	// call the routine
	RunCallInTraceback(cmdStr, 6, 0, errfunc);
	return;
}


void CLuaHandle::UnitCmdDone(const CUnit* unit, int cmdID, int cmdTag)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 8);

	int errfunc = SetupTraceback();

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		// remove error handler
		if (errfunc) lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, cmdID);
	lua_pushnumber(L, cmdTag);

	// call the routine
	RunCallInTraceback(cmdStr, 5, 0, errfunc);
	return;
}


void CLuaHandle::UnitDamaged(const CUnit* unit, const CUnit* attacker,
                             float damage, int weaponID, bool paralyzer)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 11);

	int errfunc = SetupTraceback();

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		// remove error handler
		if (errfunc) lua_pop(L, 1);
		return; // the call is not defined
	}

	int argCount = 5;
	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, damage);
	lua_pushboolean(L, paralyzer);
	if (fullRead) {
		lua_pushnumber(L, weaponID);
		argCount += 1;
		if (attacker != NULL) {
			lua_pushnumber(L, attacker->id);
			lua_pushnumber(L, attacker->unitDef->id);
			lua_pushnumber(L, attacker->team);
			argCount += 3;
		}
	}

	// call the routine
	RunCallInTraceback(cmdStr, argCount, 0, errfunc);
	return;
}


void CLuaHandle::UnitExperience(const CUnit* unit, float oldExperience)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 8);

	int errfunc = SetupTraceback();

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		// remove error handler
		if (errfunc) lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, unit->experience);
	lua_pushnumber(L, oldExperience);

	// call the routine
	RunCallInTraceback(cmdStr, 5, 0, errfunc);
	return;
}


/******************************************************************************/

void CLuaHandle::UnitSeismicPing(const CUnit* unit, int allyTeam,
                                 const float3& pos, float strength)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 9);
	if ((readAllyTeam >= 0) && (unit->losStatus[readAllyTeam] & LOS_INLOS)) {
		return; // don't need to see this ping
	}

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
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
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 6);
	if (!hs.GetGlobalFunc(L)) {
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
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 8);

	int errfunc = SetupTraceback();

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		// remove error handler
		if (errfunc) lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, transport->id);
	lua_pushnumber(L, transport->team);

	// call the routine
	RunCallInTraceback(cmdStr, 5, 0, errfunc);
	return;
}


void CLuaHandle::UnitUnloaded(const CUnit* unit, const CUnit* transport)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 8);

	int errfunc = SetupTraceback();

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		// remove error handler
		if (errfunc) lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, transport->id);
	lua_pushnumber(L, transport->team);

	// call the routine
	RunCallInTraceback(cmdStr, 5, 0, errfunc);
	return;
}


/******************************************************************************/

void CLuaHandle::UnitEnteredWater(const CUnit* unit)
{
	static const LuaHashString cmdStr(__FUNCTION__);
	UnitCallIn(cmdStr, unit);
	return;
}


void CLuaHandle::UnitEnteredAir(const CUnit* unit)
{
	static const LuaHashString cmdStr(__FUNCTION__);
	UnitCallIn(cmdStr, unit);
	return;
}


void CLuaHandle::UnitLeftWater(const CUnit* unit)
{
	static const LuaHashString cmdStr(__FUNCTION__);
	UnitCallIn(cmdStr, unit);
	return;
}


void CLuaHandle::UnitLeftAir(const CUnit* unit)
{
	static const LuaHashString cmdStr(__FUNCTION__);
	UnitCallIn(cmdStr, unit);
	return;
}


/******************************************************************************/

void CLuaHandle::UnitCloaked(const CUnit* unit)
{
	static const LuaHashString cmdStr(__FUNCTION__);
	UnitCallIn(cmdStr, unit);
	return;
}


void CLuaHandle::UnitDecloaked(const CUnit* unit)
{
	static const LuaHashString cmdStr(__FUNCTION__);
	UnitCallIn(cmdStr, unit);
	return;
}


void CLuaHandle::UnitMoveFailed(const CUnit* unit)
{
	static const LuaHashString cmdStr(__FUNCTION__);
	UnitCallIn(cmdStr, unit);
	return;
}


/******************************************************************************/

void CLuaHandle::FeatureCreated(const CFeature* feature)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);

	int errfunc = SetupTraceback();

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		// remove error handler
		if (errfunc) lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, feature->id);
	lua_pushnumber(L, feature->allyteam);

	// call the routine
	RunCallInTraceback(cmdStr, 2, 0, errfunc);
	return;
}


void CLuaHandle::FeatureDestroyed(const CFeature* feature)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);

	int errfunc = SetupTraceback();

	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		// remove error handler
		if (errfunc) lua_pop(L, 1);
		return; // the call is not defined
	}

	lua_pushnumber(L, feature->id);
	lua_pushnumber(L, feature->allyteam);

	// call the routine
	RunCallInTraceback(cmdStr, 2, 0, errfunc);
	return;
}


/******************************************************************************/

void CLuaHandle::ProjectileCreated(const CProjectile* p)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		return; // the call is not defined
	}

	if (p->synced && (p->weapon || p->piece)) {
		const CUnit* owner = p->owner();

		lua_pushnumber(L, p->id);
		lua_pushnumber(L, (owner? owner->id: -1));

		// call the routine
		RunCallIn(cmdStr, 2, 0);
	}

	return;
}


void CLuaHandle::ProjectileDestroyed(const CProjectile* p)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		return; // the call is not defined
	}

	if (p->synced && (p->weapon || p->piece)) {
		lua_pushnumber(L, p->id);

		// call the routine
		RunCallIn(cmdStr, 1, 0);
	}

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

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 7);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
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
	if (!lua_isboolean(L, -1)) {
		logOutput.Print("%s() bad return value\n", cmdStr.GetString().c_str());
		lua_pop(L, 1);
		return false;
	}

	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


void CLuaHandle::StockpileChanged(const CUnit* unit,
                                  const CWeapon* weapon, int oldCount)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 8);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		return;
	}

	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, unit->unitDef->id);
	lua_pushnumber(L, unit->team);
	lua_pushnumber(L, weapon->weaponNum);
	lua_pushnumber(L, oldCount);
	lua_pushnumber(L, weapon->numStockpiled);

	// call the routine
	RunCallIn(cmdStr, 6, 0);

	return;
}


bool CLuaHandle::RecvLuaMsg(const string& msg, int playerID)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 8);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!cmdStr.GetGlobalFunc(L)) {
		return false;
	}

	lua_pushlstring(L, msg.data(), msg.size()); // allow embedded 0's
	lua_pushnumber(L, playerID);

	// call the routine
	if (!RunCallIn(cmdStr, 2, 1)) {
		return false;
	}

	if (!lua_isboolean(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


/******************************************************************************/

void CLuaHandle::HandleLuaMsg(int playerID, int script, int mode, const std::vector<boost::uint8_t>& data)
{
	std::string msg;
	msg.resize(data.size());
	std::copy(data.begin(), data.end(), msg.begin());
	if (script == LUA_HANDLE_ORDER_UI) {
		if (luaUI) {
			bool sendMsg = false;
			if (mode == 0) {
				sendMsg = true;
			}
			else if (mode == 's') {
				sendMsg = gu->spectating;
			}
			else if (mode == 'a') {
				const CPlayer* player = playerHandler->Player(playerID);
				if (player == NULL) {
					return;
				}
				if (gu->spectatingFullView) {
					sendMsg = true;
				}
				else if (player->spectator) {
					sendMsg = gu->spectating;
				} else {
					const int msgAllyTeam = teamHandler->AllyTeam(player->team);
					sendMsg = teamHandler->Ally(msgAllyTeam, gu->myAllyTeam);
				}
			}
			if (sendMsg) {
				luaUI->RecvLuaMsg(msg, playerID);
			}
		}
	}
	else if (script == LUA_HANDLE_ORDER_GAIA) {
		if (luaGaia) {
			luaGaia->RecvLuaMsg(msg, playerID);
		}
	}
	else if (script == LUA_HANDLE_ORDER_RULES) {
		if (luaRules) {
			luaRules->RecvLuaMsg(msg, playerID);
		}
	}
}


/******************************************************************************/

inline bool CLuaHandle::PushUnsyncedCallIn(const LuaHashString& hs)
{
	// LuaUI keeps these call-ins in the Global table,
	// the synced handles keep them in the Registry table
	if (userMode) {
		return hs.GetGlobalFunc(L);
	} else {
		return hs.GetRegistryFunc(L);
	}
}


void CLuaHandle::Save(zipFile archive)
{
	// LuaUI does not get this call-in
	if (userMode) {
		return;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return;
	}

	// Save gets ZipFileWriter userdatum as single argument
	LuaZipFileWriter::PushNew(L, "", archive);

	// call the routine
	RunCallInUnsynced(cmdStr, 1, 0);

	return;
}


void CLuaHandle::Update()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return;
	}

	// call the routine
	RunCallInUnsynced(cmdStr, 0, 0);

	return;
}


void CLuaHandle::ViewResize()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return;
	}

	lua_newtable(L);
	LuaPushNamedNumber(L, "screenSizeX", globalRendering->screenSizeX);
	LuaPushNamedNumber(L, "screenSizeY", globalRendering->screenSizeY);
	LuaPushNamedNumber(L, "screenPosX",  0.0f);
	LuaPushNamedNumber(L, "screenPosY",  0.0f);
	LuaPushNamedNumber(L, "windowSizeX", globalRendering->winSizeX);
	LuaPushNamedNumber(L, "windowSizeY", globalRendering->winSizeY);
	LuaPushNamedNumber(L, "windowPosX",  globalRendering->winPosX);
	LuaPushNamedNumber(L, "windowPosY",  globalRendering->winPosY);
	LuaPushNamedNumber(L, "viewSizeX",   globalRendering->viewSizeX);
	LuaPushNamedNumber(L, "viewSizeY",   globalRendering->viewSizeY);
	LuaPushNamedNumber(L, "viewPosX",    globalRendering->viewPosX);
	LuaPushNamedNumber(L, "viewPosY",    globalRendering->viewPosY);

	// call the routine
	RunCallInUnsynced(cmdStr, 1, 0);

	return;
}


bool CLuaHandle::DefaultCommand(const CUnit* unit,
                                const CFeature* feature, int& cmd)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return false;
	}

	int args = 0;
	if (unit) {
		HSTR_PUSH(L, "unit");
		lua_pushnumber(L, unit->id);
		args = 2;
	}
	else if (feature) {
		HSTR_PUSH(L, "feature");
		lua_pushnumber(L, feature->id);
		args = 2;
	}
/* FIXME
	else if (groundPos) {
		HSTR_PUSH(L, "ground");
		lua_pushnumber(L, groundPos->x);
		lua_pushnumber(L, groundPos->y);
		lua_pushnumber(L, groundPos->z);
		args = 4;
	}
	else {
		HSTR_PUSH(L, "selection");
		args = 1;
	}
*/

	// call the routine
	RunCallInUnsynced(cmdStr, args, 1);

	if (!lua_isnumber(L, 1)) {
		lua_pop(L, 1);
		return false;
	}

	cmd = lua_toint(L, -1);
	lua_pop(L, 1);
	return true;
}




void CLuaHandle::DrawGenesis()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return;
	}

	// call the routine
	RunCallInUnsynced(cmdStr, 0, 0);

	return;
}


void CLuaHandle::DrawWorld()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return;
	}

	// call the routine
	RunCallInUnsynced(cmdStr, 0, 0);

	return;
}


void CLuaHandle::DrawWorldPreUnit()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return;
	}

	// call the routine
	RunCallInUnsynced(cmdStr, 0, 0);

	return;
}


void CLuaHandle::DrawWorldShadow()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return;
	}

	// call the routine
	RunCallInUnsynced(cmdStr, 0, 0);

	return;
}


void CLuaHandle::DrawWorldReflection()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return;
	}

	// call the routine
	RunCallInUnsynced(cmdStr, 0, 0);

	return;
}


void CLuaHandle::DrawWorldRefraction()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return;
	}

	// call the routine
	RunCallInUnsynced(cmdStr, 0, 0);

	return;
}


void CLuaHandle::DrawScreen()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return;
	}

	lua_pushnumber(L, globalRendering->viewSizeX);
	lua_pushnumber(L, globalRendering->viewSizeY);

	// call the routine
	RunCallInUnsynced(cmdStr, 2, 0);

	return;
}


void CLuaHandle::DrawScreenEffects()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return;
	}

	lua_pushnumber(L, globalRendering->viewSizeX);
	lua_pushnumber(L, globalRendering->viewSizeY);

	// call the routine
	RunCallInUnsynced(cmdStr, 2, 0);

	return;
}


void CLuaHandle::DrawInMiniMap()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return;
	}

	const int xSize = minimap->GetSizeX();
	const int ySize = minimap->GetSizeY();

	if ((xSize < 1) || (ySize < 1)) {
		lua_pop(L, 1);
		return;
	}

	lua_pushnumber(L, xSize);
	lua_pushnumber(L, ySize);

	// call the routine
	RunCallInUnsynced(cmdStr, 2, 0);

	return;
}

/******************************************************************************/
/******************************************************************************/

static inline bool CheckModUICtrl()
{
	return CLuaHandle::GetModUICtrl() ||
	       CLuaHandle::GetActiveHandle()->GetUserMode();
}



bool CLuaHandle::KeyPress(unsigned short key, bool isRepeat)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 6);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, key);

	lua_newtable(L);
	HSTR_PUSH_BOOL(L, "alt",   !!keys[SDLK_LALT]);
	HSTR_PUSH_BOOL(L, "ctrl",  !!keys[SDLK_LCTRL]);
	HSTR_PUSH_BOOL(L, "meta",  !!keys[SDLK_LMETA]);
	HSTR_PUSH_BOOL(L, "shift", !!keys[SDLK_LSHIFT]);

	lua_pushboolean(L, isRepeat);

	CKeySet ks(key, false);
	lua_pushstring(L, ks.GetString(true).c_str());

	lua_pushnumber(L, currentUnicode);

	// call the function
	if (!RunCallInUnsynced(cmdStr, 5, 1)) {
		return false;
	}

	// const int args = lua_gettop(L); unused
	if (!lua_isboolean(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::KeyRelease(unsigned short key)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, key);

	lua_newtable(L);
	HSTR_PUSH_BOOL(L, "alt",   !!keys[SDLK_LALT]);
	HSTR_PUSH_BOOL(L, "ctrl",  !!keys[SDLK_LCTRL]);
	HSTR_PUSH_BOOL(L, "meta",  !!keys[SDLK_LMETA]);
	HSTR_PUSH_BOOL(L, "shift", !!keys[SDLK_LSHIFT]);

	CKeySet ks(key, false);
	lua_pushstring(L, ks.GetString(true).c_str());

	lua_pushnumber(L, currentUnicode);

	// call the function
	if (!RunCallInUnsynced(cmdStr, 4, 1)) {
		return false;
	}

	if (!lua_isboolean(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::MousePress(int x, int y, int button)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, x - globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewSizeY - y - 1);
	lua_pushnumber(L, button);

	// call the function
	if (!RunCallInUnsynced(cmdStr, 3, 1)) {
		return false;
	}

	if (!lua_isboolean(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


int CLuaHandle::MouseRelease(int x, int y, int button)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, x - globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewSizeY - y - 1);
	lua_pushnumber(L, button);

	// call the function
	if (!RunCallInUnsynced(cmdStr, 3, 1)) {
		return false;
	}

	if (!lua_isnumber(L, -1)) {
		lua_pop(L, 1);
		return -1;
	}
	const int retval = lua_toint(L, -1) - 1;
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::MouseMove(int x, int y, int dx, int dy, int button)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 7);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, x - globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewSizeY - y - 1);
	lua_pushnumber(L, dx);
	lua_pushnumber(L, -dy);
	lua_pushnumber(L, button);

	// call the function
	if (!RunCallInUnsynced(cmdStr, 5, 1)) {
		return false;
	}

	if (!lua_isboolean(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::MouseWheel(bool up, float value)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return false; // the call is not defined, do not take the event
	}

	lua_pushboolean(L, up);
	lua_pushnumber(L, value);

	// call the function
	if (!RunCallInUnsynced(cmdStr, 2, 1)) {
		return false;
	}

	if (!lua_isboolean(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}

bool CLuaHandle::JoystickEvent(const std::string& event, int val1, int val2)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);
	const LuaHashString cmdStr(event);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, val1);
	lua_pushnumber(L, val2);

	// call the function
	if (!RunCallInUnsynced(cmdStr, 2, 1)) {
		return false;
	}

	if (!lua_isboolean(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}

bool CLuaHandle::IsAbove(int x, int y)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return false; // the call is not defined
	}

	lua_pushnumber(L, x - globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewSizeY - y - 1);

	// call the function
	if (!RunCallInUnsynced(cmdStr, 2, 1)) {
		return false;
	}

	if (!lua_isboolean(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


string CLuaHandle::GetTooltip(int x, int y)
{
	if (!CheckModUICtrl()) {
		return "";
	}
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return ""; // the call is not defined
	}

	lua_pushnumber(L, x - globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewSizeY - y - 1);

	// call the function
	if (!RunCallInUnsynced(cmdStr, 2, 1)) {
		return "";
	}

	if (!lua_isstring(L, -1)) {
		lua_pop(L, 1);
		return "";
	}
	const string retval = lua_tostring(L, -1);
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::ConfigCommand(const string& command)
{
	if (!CheckModUICtrl()) {
		return true; // FIXME ?
	}
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);
	static const LuaHashString cmdStr("ConfigureLayout");
	//static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return true; // the call is not defined
	}

	lua_pushstring(L, command.c_str());

	// call the routine
	if (!RunCallInUnsynced(cmdStr, 1, 0)) {
		return false;
	}
	return true;
}


bool CLuaHandle::CommandNotify(const Command& cmd)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return false; // the call is not defined
	}

	// push the command id
	lua_pushnumber(L, cmd.id);

	// push the params list
	lua_newtable(L);
	for (int p = 0; p < (int)cmd.params.size(); p++) {
		lua_pushnumber(L, p + 1);
		lua_pushnumber(L, cmd.params[p]);
		lua_rawset(L, -3);
	}

	// push the options table
	lua_newtable(L);
	HSTR_PUSH_NUMBER(L, "coded", cmd.options);
	HSTR_PUSH_BOOL(L, "alt",   !!(cmd.options & ALT_KEY));
	HSTR_PUSH_BOOL(L, "ctrl",  !!(cmd.options & CONTROL_KEY));
	HSTR_PUSH_BOOL(L, "shift", !!(cmd.options & SHIFT_KEY));
	HSTR_PUSH_BOOL(L, "right", !!(cmd.options & RIGHT_MOUSE_KEY));
	HSTR_PUSH_BOOL(L, "meta",  !!(cmd.options & META_KEY));

	// call the function
	if (!RunCallInUnsynced(cmdStr, 3, 1)) {
		return false;
	}

	// get the results
	if (!lua_isboolean(L, -1)) {
		logOutput.Print("CommandNotify() bad return value\n");
		lua_pop(L, 1);
		return false;
	}

	const bool retval = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::AddConsoleLine(const string& msg, const CLogSubsystem& /**/)
{
	if (!CheckModUICtrl()) {
		return true; // FIXME?
	}
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return true; // the call is not defined
	}

	lua_pushstring(L, msg.c_str());
	// FIXME: makes no sense now, but *gets might expect this
	lua_pushnumber(L, 0);

	// call the function
	if (!RunCallIn(cmdStr, 2, 0)) {
		return false;
	}

	return true;
}



bool CLuaHandle::GroupChanged(int groupID)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return false; // the call is not defined
	}

	lua_pushnumber(L, groupID);

	// call the routine
	if (!RunCallInUnsynced(cmdStr, 1, 0)) {
		return false;
	}

	return true;
}



string CLuaHandle::WorldTooltip(const CUnit* unit,
                                const CFeature* feature,
                                const float3* groundPos)
{
	if (!CheckModUICtrl()) {
		return "";
	}
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 6);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return ""; // the call is not defined
	}

	int args;
	if (unit) {
		HSTR_PUSH(L, "unit");
		lua_pushnumber(L, unit->id);
		args = 2;
	}
	else if (feature) {
		HSTR_PUSH(L, "feature");
		lua_pushnumber(L, feature->id);
		args = 2;
	}
	else if (groundPos) {
		HSTR_PUSH(L, "ground");
		lua_pushnumber(L, groundPos->x);
		lua_pushnumber(L, groundPos->y);
		lua_pushnumber(L, groundPos->z);
		args = 4;
	}
	else {
		HSTR_PUSH(L, "selection");
		args = 1;
	}

	// call the routine
	if (!RunCallInUnsynced(cmdStr, args, 1)) {
		return "";
	}

	if (!lua_isstring(L, -1)) {
		lua_pop(L, 1);
		return "";
	}
	const string retval = lua_tostring(L, -1);
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::MapDrawCmd(int playerID, int type,
                            const float3* pos0,
                            const float3* pos1,
                            const string* label)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 9);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return false; // the call is not defined
	}

	int args;

	lua_pushnumber(L, playerID);

	if (type == MAPDRAW_POINT) {
		HSTR_PUSH(L, "point");
		lua_pushnumber(L, pos0->x);
		lua_pushnumber(L, pos0->y);
		lua_pushnumber(L, pos0->z);
		lua_pushstring(L, label->c_str());
		args = 6;
	}
	else if (type == MAPDRAW_LINE) {
		HSTR_PUSH(L, "line");
		lua_pushnumber(L, pos0->x);
		lua_pushnumber(L, pos0->y);
		lua_pushnumber(L, pos0->z);
		lua_pushnumber(L, pos1->x);
		lua_pushnumber(L, pos1->y);
		lua_pushnumber(L, pos1->z);
		args = 8;
	}
	else if (type == MAPDRAW_ERASE) {
		HSTR_PUSH(L, "erase");
		lua_pushnumber(L, pos0->x);
		lua_pushnumber(L, pos0->y);
		lua_pushnumber(L, pos0->z);
		lua_pushnumber(L, 100.0f);  // radius
		args = 6;
	}
	else {
		logOutput.Print("Unknown MapDrawCmd() type: %i", type);
		lua_pop(L, 2); // pop the function and playerID
		return false;
	}

	// call the routine
	if (!RunCallInUnsynced(cmdStr, args, 1)) {
		return false;
	}

	// take the event?
	if (!lua_isboolean(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	const bool retval = lua_toboolean(L, -1);
	lua_pop(L, 1);
	return retval;
}


bool CLuaHandle::GameSetup(const string& state, bool& ready,
                           const map<int, string>& playerStates)
{
	if (!CheckModUICtrl()) {
		return false;
	}
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushUnsyncedCallIn(cmdStr)) {
		return false;
	}

	lua_pushstring(L, state.c_str());

	lua_pushboolean(L, ready);

	lua_newtable(L);
	map<int, string>::const_iterator it;
	for (it = playerStates.begin(); it != playerStates.end(); ++it) {
		lua_pushnumber(L, it->first);
		lua_pushstring(L, it->second.c_str());
		lua_rawset(L, -3);
	}

	// call the routine
	if (!RunCallInUnsynced(cmdStr, 3, 2)) {
		return false;
	}

	if (lua_isboolean(L, -2)) {
		if (lua_toboolean(L, -2)) {
			if (lua_isboolean(L, -1)) {
				ready = lua_toboolean(L, -1);
			}
			lua_pop(L, 2);
			return true;
		}
	}
	lua_pop(L, 2);
	return false;
}


/******************************************************************************/
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
		HSTR_PUSH_CFUNC(L, "GetCallInList",   CallOutGetCallInList);
		// special team constants
		HSTR_PUSH_NUMBER(L, "NO_ACCESS_TEAM",  CEventClient::NoAccessTeam);
		HSTR_PUSH_NUMBER(L, "ALL_ACCESS_TEAM", CEventClient::AllAccessTeam);
//FIXME		LuaArrays::PushEntries(L);
	}
	lua_rawset(L, -3);

	// extra math utilities
	lua_getglobal(L, "math");
	LuaBitOps::PushEntries(L);
	lua_pop(L, 1);
	return true;
}


int CLuaHandle::CallOutGetName(lua_State* L)
{
	lua_pushstring(L, activeHandle->GetName().c_str());
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


int CLuaHandle::CallOutGetCallInList(lua_State* L)
{
	vector<string> list;
	eventHandler.GetEventList(list);
	lua_createtable(L, 0, list.size());
	for (unsigned int i = 0; i < list.size(); i++) {
		lua_pushstring(L, list[i].c_str());
		lua_newtable(L); {
			lua_pushliteral(L, "unsynced");
			lua_pushboolean(L, eventHandler.IsUnsynced(list[i]));
			lua_rawset(L, -3);
			lua_pushliteral(L, "controller");
			lua_pushboolean(L, eventHandler.IsController(list[i]));
			lua_rawset(L, -3);
		}
		lua_rawset(L, -3);
	}
	return 1;
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
