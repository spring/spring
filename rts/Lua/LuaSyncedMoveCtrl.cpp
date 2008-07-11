#include "StdAfx.h"
// LuaSyncedMoveCtrl.cpp.cpp: implementation of the CLuaSyncedMoveCtrl class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaSyncedMoveCtrl.h"
#include <set>
#include <list>
#include <cctype>

#include "LuaInclude.h"

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

	REGISTER_LUA_CFUNC(SetPhysics);
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
	REGISTER_LUA_CFUNC(SetDrag);

	REGISTER_LUA_CFUNC(SetWindFactor);

	REGISTER_LUA_CFUNC(SetLimits);

	REGISTER_LUA_CFUNC(SetNoBlocking);

	REGISTER_LUA_CFUNC(SetLeaveTracks);

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
		return (ctrlTeam == CEventClient::AllAccessTeam) ? true : false;
	}
	return (ctrlTeam == unit->team);
}


static inline CUnit* ParseUnit(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "%s(): Bad unitID", caller);
	}
	const int unitID = lua_toint(L, index);
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
	moveType->tag = lua_toint(L, 2);
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
		const int state = lua_toint(L, 2);
		if ((state < AMoveType::Done) || (state > AMoveType::Failed)) {
			luaL_error(L, "SetProgressState(): bad state value (%i)", state);
		}
		moveType->progressState = (AMoveType::ProgressState) state;
	}
	else if (lua_isstring(L, 2)) {
		const string state = lua_tostring(L, 2);
		if (state == "done") {
			moveType->progressState = AMoveType::Done;
		} else if (state == "active") {
			moveType->progressState = AMoveType::Active;
		} else if (state == "failed") {
			moveType->progressState = AMoveType::Failed;
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
	const float3 pos(luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3),
	                 luaL_checkfloat(L, 4));
	const float3 vel(luaL_checkfloat(L, 5),
	                 luaL_checkfloat(L, 6),
	                 luaL_checkfloat(L, 7));
	const float3 rot(luaL_checkfloat(L, 8),
	                 luaL_checkfloat(L, 9),
	                 luaL_checkfloat(L, 10));
	moveType->SetPhysics(pos, vel, rot);
	return 0;
}


int LuaSyncedMoveCtrl::SetPosition(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const float3 pos(luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3),
	                 luaL_checkfloat(L, 4));
	moveType->SetPosition(pos);
	return 0;
}


int LuaSyncedMoveCtrl::SetVelocity(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const float3 vel(luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3),
	                 luaL_checkfloat(L, 4));
	moveType->SetVelocity(vel);
	return 0;
}


int LuaSyncedMoveCtrl::SetRelativeVelocity(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const float3 relVel(luaL_checkfloat(L, 2),
	                    luaL_checkfloat(L, 3),
	                    luaL_checkfloat(L, 4));
	moveType->SetRelativeVelocity(relVel);
	return 0;
}


int LuaSyncedMoveCtrl::SetRotation(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const float3 rot(luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3),
	                 luaL_checkfloat(L, 4));
	moveType->SetRotation(rot);
	return 0;
}


int LuaSyncedMoveCtrl::SetRotationOffset(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const float3 rotOff(luaL_checkfloat(L, 2),
	                    luaL_checkfloat(L, 3),
	                    luaL_checkfloat(L, 4));
	moveType->SetRotationOffset(rotOff);
	return 0;
}


int LuaSyncedMoveCtrl::SetRotationVelocity(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const float3 rotVel(luaL_checkfloat(L, 2),
	                    luaL_checkfloat(L, 3),
	                    luaL_checkfloat(L, 4));
	moveType->SetRotationVelocity(rotVel);
	return 0;
}


int LuaSyncedMoveCtrl::SetHeading(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const short heading = (short)luaL_checknumber(L, 2);
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
	moveType->groundOffset = luaL_checkfloat(L, 2);
	return 0;
}


int LuaSyncedMoveCtrl::SetGravity(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	moveType->gravityFactor = luaL_checkfloat(L, 2);
	return 0;
}


int LuaSyncedMoveCtrl::SetDrag(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	moveType->drag = luaL_checkfloat(L, 2);
	return 0;
}


int LuaSyncedMoveCtrl::SetWindFactor(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	moveType->windFactor = luaL_checkfloat(L, 2);
	return 0;
}


int LuaSyncedMoveCtrl::SetLimits(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const float3 mins(luaL_checkfloat(L, 2),
	                  luaL_checkfloat(L, 3),
	                  luaL_checkfloat(L, 4));
	const float3 maxs(luaL_checkfloat(L, 5),
	                  luaL_checkfloat(L, 6),
	                  luaL_checkfloat(L, 7));
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


int LuaSyncedMoveCtrl::SetLeaveTracks(lua_State* L)
{
	CScriptMoveType* moveType = ParseMoveType(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isboolean(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetLeaveTracks()");
	}
	moveType->leaveTracks = lua_toboolean(L, 2);
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
	moveType->gndStop = lua_toboolean(L, 2); // FIXME
	moveType->collideStop = lua_toboolean(L, 2);
	return 0;
}


/******************************************************************************/
/******************************************************************************/
