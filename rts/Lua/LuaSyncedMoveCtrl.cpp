#include "StdAfx.h"
// LuaSyncedMoveCtrl.cpp.cpp: implementation of the CLuaSyncedMoveCtrl class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaSyncedMoveCtrl.h"
#include <set>
#include <list>
#include <cctype>

extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaUtils.h"
#include "Sim/MoveTypes/ScriptMoveType.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "System/myMath.h"
#include "System/LogOutput.h"

using namespace std;


/******************************************************************************/
/******************************************************************************/


bool LuaSyncedMoveCtrl::PushMoveCtrl(lua_State* L)
{
	lua_pushstring(L, "MoveCtrl");
	lua_newtable(L);

#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(IsEnabled);

	REGISTER_LUA_CFUNC(Enable);
	REGISTER_LUA_CFUNC(Disable);

	REGISTER_LUA_CFUNC(SetTag);
	REGISTER_LUA_CFUNC(GetTag);

	REGISTER_LUA_CFUNC(SetProgressState);

	REGISTER_LUA_CFUNC(SetExtrapolate);

	REGISTER_LUA_CFUNC(SetPosition);
	REGISTER_LUA_CFUNC(SetVelocity);
	REGISTER_LUA_CFUNC(SetRelativeVelocity);
	REGISTER_LUA_CFUNC(SetRotation);
	REGISTER_LUA_CFUNC(SetRotationVelocity);
	REGISTER_LUA_CFUNC(SetRotationOffset);
	REGISTER_LUA_CFUNC(SetHeading);

	REGISTER_LUA_CFUNC(SetTrackSlope);
	REGISTER_LUA_CFUNC(SetTrackGround);
	REGISTER_LUA_CFUNC(SetGroundOffset);
	REGISTER_LUA_CFUNC(SetGravity);

	REGISTER_LUA_CFUNC(SetWindFactor);

	REGISTER_LUA_CFUNC(SetLimits);

	REGISTER_LUA_CFUNC(SetNoBlocking);

	REGISTER_LUA_CFUNC(SetShotStop);
	REGISTER_LUA_CFUNC(SetSlopeStop);
	REGISTER_LUA_CFUNC(SetCollideStop);

	lua_rawset(L, -3);
	
	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Access helpers
//

static inline int CtrlTeam()
{
	return CLuaHandle::GetActiveHandle()->GetCtrlTeam();
}


static inline bool CanControlUnit(const CUnit* unit)
{
	const int ctrlTeam = CtrlTeam();
	if (ctrlTeam < 0) {
		return (ctrlTeam == CLuaHandle::AllAccessTeam) ? true : false;
	}
	return (ctrlTeam == unit->team);
}


static inline CUnit* ParseUnit(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "%s(): Bad unitID", caller);
	}
	const int unitID = (int)lua_tonumber(L, index);
	if ((unitID < 0) || (unitID >= MAX_UNITS)) {
		luaL_error(L, "%s(): Bad unitID: %i\n", caller, unitID);
	}
	CUnit* unit = uh->units[unitID];
	if (unit == NULL) {
		return NULL;
	}
	if (!CanControlUnit(unit)) {
		return NULL;
	}
	return unit;
}


static inline CUnit* ParseControlledUnit(lua_State* L,
                                         const char* caller, int index)
{
	CUnit* unit = ParseUnit(L, caller, index);
	if (unit == NULL) {
		return NULL;
	}
	if (!unit->usingScriptMoveType) {
		return NULL;
	}
	return unit;
}


static inline CScriptMoveType* ParseMoveType(lua_State* L,
                                             const char* caller, int index)
{
	CUnit* unit = ParseUnit(L, caller, index);
	if (unit == NULL) {
		return NULL;
	}
	if (!unit->usingScriptMoveType) {
		return NULL;
	}
	return (CScriptMoveType*)unit->moveType;
}


/******************************************************************************/
/******************************************************************************/

int LuaSyncedMoveCtrl::IsEnabled(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushboolean(L, unit->usingScriptMoveType);
	return 1;
}


int LuaSyncedMoveCtrl::Enable(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	unit->EnableScriptMoveType();
	return 0;
}


int LuaSyncedMoveCtrl::Disable(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	unit->DisableScriptMoveType();
	return 0;
}


/******************************************************************************/

int LuaSyncedMoveCtrl::SetTag(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetTag()");
	}
	moveType->tag = (int)lua_tonumber(L, 2);
	return 0;
}


int LuaSyncedMoveCtrl::GetTag(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	lua_pushnumber(L, moveType->tag);
	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaSyncedMoveCtrl::SetProgressState(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if (args < 2) {
		luaL_error(L, "Incorrect arguments to SetProgressState()");
	}

	if (lua_isnumber(L, 2)) {
		const int state = (int)lua_tonumber(L, 2);
		if ((state < CMoveType::Done) || (state > CMoveType::Failed)) {
			luaL_error(L, "SetProgressState(): bad state value (%i)", state);
		}
		moveType->progressState = (CMoveType::ProgressState) state;
	}
	else if (lua_isstring(L, 2)) {
		const string state = lua_tostring(L, 2);
		if (state == "done") {
			moveType->progressState = CMoveType::Done;
		} else if (state == "active") {
			moveType->progressState = CMoveType::Active;
		} else if (state == "failed") {
			moveType->progressState = CMoveType::Failed;
		} else {
			luaL_error(L, "SetProgressState(): bad state value (%s)", state.c_str());
		}
	}
	else {
		luaL_error(L, "Incorrect arguments to SetProgressState()");
	}
	return 0;
}


/******************************************************************************/

int LuaSyncedMoveCtrl::SetExtrapolate(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isboolean(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetExtrapolate()");
	}
	moveType->extrapolate = lua_toboolean(L, 2);
	return 0;
}


/******************************************************************************/

int LuaSyncedMoveCtrl::SetPhysics(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 10) ||
	    !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4) ||
	    !lua_isnumber(L, 5) || !lua_isnumber(L, 6) || !lua_isnumber(L, 7) ||
	    !lua_isnumber(L, 8) || !lua_isnumber(L, 9) || !lua_isnumber(L, 10)) {
		luaL_error(L, "Incorrect arguments to SetPhysics()");
	}
	const float3 pos((float)lua_tonumber(L, 2),
	                 (float)lua_tonumber(L, 3),
	                 (float)lua_tonumber(L, 4));
	const float3 vel((float)lua_tonumber(L, 5),
	                 (float)lua_tonumber(L, 6),
	                 (float)lua_tonumber(L, 7));
	const float3 rot((float)lua_tonumber(L, 8),
	                 (float)lua_tonumber(L, 9),
	                 (float)lua_tonumber(L, 10));
	moveType->SetPhysics(pos, vel, rot);
	return 0;
}


int LuaSyncedMoveCtrl::SetPosition(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 4) ||
	    !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		luaL_error(L, "Incorrect arguments to SetPosition()");
	}
	const float3 pos((float)lua_tonumber(L, 2),
	                 (float)lua_tonumber(L, 3),
	                 (float)lua_tonumber(L, 4));
	moveType->SetPosition(pos);
	return 0;
}


int LuaSyncedMoveCtrl::SetVelocity(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 4) ||
	    !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		luaL_error(L, "Incorrect arguments to SetVelocity()");
	}
	const float3 vel((float)lua_tonumber(L, 2),
	                 (float)lua_tonumber(L, 3),
	                 (float)lua_tonumber(L, 4));
	moveType->SetVelocity(vel);
	return 0;
}


int LuaSyncedMoveCtrl::SetRelativeVelocity(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 4) ||
	    !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		luaL_error(L, "Incorrect arguments to SetVelocity()");
	}
	const float3 relVel((float)lua_tonumber(L, 2),
	                    (float)lua_tonumber(L, 3),
	                    (float)lua_tonumber(L, 4));
	moveType->SetRelativeVelocity(relVel);
	return 0;
}


int LuaSyncedMoveCtrl::SetRotation(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 4) ||
	    !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		luaL_error(L, "Incorrect arguments to SetRotation()");
	}
	const float3 rot((float)lua_tonumber(L, 2),
	                 (float)lua_tonumber(L, 3),
	                 (float)lua_tonumber(L, 4));
	moveType->SetRotation(rot);
	return 0;
}


int LuaSyncedMoveCtrl::SetRotationOffset(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 4) ||
	    !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		luaL_error(L, "Incorrect arguments to SetRotationOffset()");
	}
	const float3 rotOff((float)lua_tonumber(L, 2),
	                    (float)lua_tonumber(L, 3),
	                    (float)lua_tonumber(L, 4));
	moveType->SetRotationOffset(rotOff);
	return 0;
}


int LuaSyncedMoveCtrl::SetRotationVelocity(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 4) ||
	    !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		luaL_error(L, "Incorrect arguments to SetRotationVelocity()");
	}
	const float3 rotVel((float)lua_tonumber(L, 2),
	                    (float)lua_tonumber(L, 3),
	                    (float)lua_tonumber(L, 4));
	moveType->SetRotationVelocity(rotVel);
	return 0;
}


int LuaSyncedMoveCtrl::SetHeading(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetHeading()");
	}
	const short heading = (short)lua_tonumber(L, 2);
	moveType->SetHeading(heading);
	return 0;
}


/******************************************************************************/

int LuaSyncedMoveCtrl::SetTrackSlope(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isboolean(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetTrackSlope()");
	}
	moveType->trackSlope = lua_toboolean(L, 2);
	return 0;
}


int LuaSyncedMoveCtrl::SetTrackGround(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isboolean(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetTrackGround()");
	}
	moveType->trackGround = lua_toboolean(L, 2);
	return 0;
}


int LuaSyncedMoveCtrl::SetGroundOffset(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetGroundOffset()");
	}
	moveType->groundOffset = (float)lua_tonumber(L, 2);
	return 0;
}


int LuaSyncedMoveCtrl::SetGravity(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetGravity()");
	}
	moveType->gravityFactor = (float)lua_tonumber(L, 2);
	return 0;
}


int LuaSyncedMoveCtrl::SetWindFactor(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetWindFactor()");
	}
	moveType->windFactor = (float)lua_tonumber(L, 2);
	return 0;
}


int LuaSyncedMoveCtrl::SetLimits(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 7) ||
	    !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4) ||
	    !lua_isnumber(L, 5) || !lua_isnumber(L, 6) || !lua_isnumber(L, 7)) {
		luaL_error(L, "Incorrect arguments to SetLimits()");
	}
	const float3 mins((float)lua_tonumber(L, 2),
	                  (float)lua_tonumber(L, 3),
	                  (float)lua_tonumber(L, 4));
	const float3 maxs((float)lua_tonumber(L, 5),
	                  (float)lua_tonumber(L, 6),
	                  (float)lua_tonumber(L, 7));
	moveType->mins = mins;
	moveType->maxs = maxs;
	return 0;
}


/******************************************************************************/

int LuaSyncedMoveCtrl::SetNoBlocking(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isboolean(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetNoBlocking()");
	}
	moveType->SetNoBlocking(lua_toboolean(L, 2));
	return 0;
}


int LuaSyncedMoveCtrl::SetShotStop(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isboolean(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetShotStop()");
	}
	moveType->shotStop = lua_toboolean(L, 2);
	return 0;
}


int LuaSyncedMoveCtrl::SetSlopeStop(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isboolean(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetSlopeStop()");
	}
	moveType->slopeStop = lua_toboolean(L, 2);
	return 0;
}


int LuaSyncedMoveCtrl::SetCollideStop(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isboolean(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetCollideStop()");
	}
	moveType->collideStop = lua_toboolean(L, 2);
}


/******************************************************************************/
/******************************************************************************/
