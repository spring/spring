/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaSyncedMoveCtrl.h"

#include "LuaInclude.h"

#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaHelper.h"
#include "LuaUtils.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/ScriptMoveType.h"
#include "Sim/MoveTypes/GroundMoveType.h"
#include "Sim/MoveTypes/AAirMoveType.h"
#include "Sim/MoveTypes/StrafeAirMoveType.h"
#include "Sim/MoveTypes/HoverAirMoveType.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "System/myMath.h"
#include "System/Log/ILog.h"

#include <cctype>


/******************************************************************************/
/******************************************************************************/


bool LuaSyncedMoveCtrl::PushMoveCtrl(lua_State* L)
{
	lua_pushliteral(L, "MoveCtrl");
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

	REGISTER_LUA_CFUNC(SetShotStop);
	REGISTER_LUA_CFUNC(SetSlopeStop);
	REGISTER_LUA_CFUNC(SetCollideStop);

	REGISTER_LUA_CFUNC(SetAirMoveTypeData);
	REGISTER_LUA_CFUNC(SetGroundMoveTypeData);
	REGISTER_LUA_CFUNC(SetGunshipMoveTypeData);

	REGISTER_LUA_CFUNC(SetMoveDef);

	lua_rawset(L, -3);

	return true;
}


static inline CUnit* ParseUnit(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "%s(): Bad unitID", caller);
	}
	const int unitID = lua_toint(L, index);
	if ((unitID < 0) || (static_cast<size_t>(unitID) >= unitHandler->MaxUnits())) {
		luaL_error(L, "%s(): Bad unitID: %i\n", caller, unitID);
	}
	CUnit* unit = unitHandler->units[unitID];
	if (unit == NULL) {
		return NULL;
	}
	if (!CanControlUnit(L, unit)) {
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
	return static_cast<CScriptMoveType*>(unit->moveType);
}


template<typename MoveType>
static inline MoveType* ParseMoveType(lua_State* L,
					      const char* caller, int index)
{
	CUnit* unit = ParseUnit(L, caller, index);
	if (unit == NULL) {
		return NULL;
	}

	MoveType* mt = dynamic_cast<MoveType*>(unit->moveType);
	return mt;
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
	ASSERT_SYNCED(pos);
	ASSERT_SYNCED(vel);
	ASSERT_SYNCED(rot);
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
	ASSERT_SYNCED(pos);
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
	ASSERT_SYNCED(vel);
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
	ASSERT_SYNCED(relVel);
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
	ASSERT_SYNCED(rot);
	moveType->SetRotation(rot);
	return 0;
}


int LuaSyncedMoveCtrl::SetRotationOffset(lua_State* L)
{
	// DEPRECATED
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
	ASSERT_SYNCED(rotVel);
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
	ASSERT_SYNCED((short)heading);
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

	// marks or unmarks the unit on the blocking-map, but
	// does not change its blocking (collidable) state
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
	moveType->gndStop = lua_toboolean(L, 2); // FIXME
	moveType->collideStop = lua_toboolean(L, 2);
	return 0;
}

/******************************************************************************/
/******************************************************************************/
/* MoveType-specific methods */

static inline bool SetGenericMoveTypeValue(AMoveType* mt, const string& key, float value)
{
	// can't set goal here, need a different function that calls mt->SetGoal
	// FIXME should use setter methods in all Set*MoveTypeValue functions, but they mostly don't exist
	if (key == "maxSpeed") {
		mt->SetMaxSpeed(value / GAME_SPEED); return true;
	} else if (key == "repairBelowHealth") {
		mt->SetRepairBelowHealth(value); return true;
	}
	return false;
}

static inline bool SetGenericMoveTypeValue(AMoveType* mt, const string& key, bool value)
{
	return false;
}


/******************************************************************************/
/* CStrafeAirMoveType handling */

static inline bool SetAirMoveTypeValue(CStrafeAirMoveType* mt, const string& key, float value)
{
	if (SetGenericMoveTypeValue(mt, key, value))
		return true;
	if (key == "wantedHeight") {
		mt->wantedHeight = value; return true;
	} else if (key == "myGravity") {
		mt->myGravity = value; return true;
	} else if (key == "maxBank") {
		mt->maxBank = value; return true;
	} else if (key == "maxPitch") {
		mt->maxPitch = value; return true;
	} else if (key == "turnRadius") {
		mt->turnRadius = value; return true;
	} else if (key == "maxAcc") {
		mt->maxAcc = value; return true;
	} else if (key == "maxAileron") {
		mt->maxAileron = value; return true;
	} else if (key == "maxElevator") {
		mt->maxElevator = value; return true;
	} else if (key == "maxRudder") {
		mt->maxRudder = value; return true;
	}

	return false;
}

static inline bool SetAirMoveTypeValue(CStrafeAirMoveType* mt, const string& key, bool value)
{
	if (SetGenericMoveTypeValue(mt, key, value))
		return true;
	if (key == "collide") {
		mt->collide = value; return true;
	} else if (key == "useSmoothMesh") {
		mt->useSmoothMesh = value; return true;
	}
	return false;
}


static inline void SetSingleAirMoveTypeValue(lua_State* L, int keyidx, int validx, CStrafeAirMoveType* moveType)
{
	const string key = lua_tostring(L, keyidx);
	bool assigned = true;

	if (lua_isnumber(L, validx)) {
		assigned = SetAirMoveTypeValue(moveType, key, lua_tofloat(L, validx));
	} else if (lua_isboolean(L, validx)) {
		assigned = SetAirMoveTypeValue(moveType, key, lua_toboolean(L, validx));
	}

	if (!assigned) {
		LOG_L(L_WARNING, "Can not assign key \"%s\" to AirMoveType", key.c_str());
	}
}

int LuaSyncedMoveCtrl::SetAirMoveTypeData(lua_State* L)
{
	CStrafeAirMoveType* moveType = ParseMoveType<CStrafeAirMoveType>(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		luaL_error(L, "Unit does not have a compatible MoveType");
	}

	const int args = lua_gettop(L); // number of arguments

	if (args == 3 && lua_isstring(L, 2)) {
		// a single value
		SetSingleAirMoveTypeValue(L, 2, 3, moveType);
	} else if (args == 2 && lua_istable(L, 2)) {
		// a table of values
		const int table = 2;
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (lua_israwstring(L, -2)) {
				SetSingleAirMoveTypeValue(L, -2, -1, moveType);
			}
		}
	}

	return 0;
}


/******************************************************************************/
/* CGroundMoveType handling */

static inline bool SetGroundMoveTypeValue(CGroundMoveType* mt, const string& key, float value)
{
	if (SetGenericMoveTypeValue(mt, key, value))
		return true;

	if (key == "turnRate") {
		mt->turnRate = value; return true;
	} else if (key == "accRate") {
		mt->accRate = value; return true;
	} else if (key == "decRate") {
		mt->decRate = value; return true;
	} else if (key == "maxReverseSpeed") {
		// use setter?
		mt->maxReverseSpeed = value / GAME_SPEED;
		return true;
	}

	return false;
}

static inline bool SetGroundMoveTypeValue(CGroundMoveType* mt, const string& key, bool value)
{
	if (SetGenericMoveTypeValue(mt, key, value))
		return true;

	return false;
}

static inline void SetSingleGroundMoveTypeValue(lua_State* L, int keyidx, int validx, CGroundMoveType* moveType)
{
	const string key = lua_tostring(L, keyidx);
	bool assigned = true;

	if (lua_isnumber(L, validx)) {
		assigned = SetGroundMoveTypeValue(moveType, key, lua_tofloat(L, validx));
	} else if (lua_isboolean(L, validx)) {
		assigned = SetGroundMoveTypeValue(moveType, key, lua_toboolean(L, validx));
	}

	if (!assigned) {
		LOG_L(L_WARNING, "Can not assign key \"%s\" to GroundMoveType", key.c_str());
	}
}

int LuaSyncedMoveCtrl::SetGroundMoveTypeData(lua_State *L)
{
	CGroundMoveType* moveType = ParseMoveType<CGroundMoveType>(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		luaL_error(L, "Unit does not have a compatible MoveType");
	}

	const int args = lua_gettop(L); // number of arguments

	if (args == 3 && lua_isstring(L, 2)) {
		// a single value
		SetSingleGroundMoveTypeValue(L, 2, 3, moveType);
	} else if (args == 2 && lua_istable(L, 2)) {
		// a table of values
		const int table = 2;
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (lua_israwstring(L, -2)) {
				SetSingleGroundMoveTypeValue(L, -2, -1, moveType);
			}
		}
	}

	return 0;
}



/******************************************************************************/
/* CHoverAirMoveType handling */

static inline bool SetHoverAirMoveTypeValue(CHoverAirMoveType* mt, const string& key, float value)
{
	if (SetGenericMoveTypeValue(mt, key, value)) {
		if (key == "maxSpeed") {
			mt->brakeDistance = (mt->GetMaxSpeed() * mt->GetMaxSpeed()) / mt->decRate;
		}
		return true;
	}

	if (key == "wantedHeight") {
		mt->SetDefaultAltitude(value); return true;
	} else if (key == "turnRate") {
		mt->turnRate = value; return true;
	} else if (key == "accRate") {
		mt->accRate = value; return true;
	} else if (key == "decRate") {
		mt->decRate = value;
		mt->brakeDistance = (mt->GetMaxSpeed() * mt->GetMaxSpeed()) / mt->decRate;
		return true;
	} else if (key == "altitudeRate") {
		mt->altitudeRate = value; return true;
	} else if (key == "currentBank") {
		mt->currentBank = value; return true;
	} else if (key == "currentPitch") {
		mt->currentPitch = value; return true;
	} else if (key == "brakeDistance") {
		mt->brakeDistance = value; return true;
	} else if (key == "maxDrift") {
		mt->maxDrift = value; return true;
	}

	return false;
}

static inline bool SetHoverAirMoveTypeValue(CHoverAirMoveType* mt, const string& key, bool value)
{
	if (SetGenericMoveTypeValue(mt, key, value))
		return true;

	if (key == "collide") {
		mt->collide = value; return true;
	} else if (key == "useSmoothMesh") {
		mt->useSmoothMesh = value; return true;
	} else if (key == "bankingAllowed") {
		mt->bankingAllowed = value; return true;
	} else if (key == "airStrafe") {
		mt->airStrafe = value; return true;
	} else if (key == "dontLand") {
		mt->dontLand = value; return true;
	}

	return false;
}

static inline void SetSingleHoverAirMoveTypeValue(lua_State* L, int keyIdx, int valIdx, CHoverAirMoveType* moveType)
{
	const string key = lua_tostring(L, keyIdx);
	bool assigned = true;

	if (lua_isnumber(L, valIdx)) {
		assigned = SetHoverAirMoveTypeValue(moveType, key, lua_tofloat(L, valIdx));
	} else if (lua_isboolean(L, valIdx)) {
		assigned = SetHoverAirMoveTypeValue(moveType, key, lua_toboolean(L, valIdx));
	}

	if (!assigned) {
		LOG_L(L_WARNING, "Can not assign key \"%s\" to GunshipMoveType", key.c_str());
	}
}

int LuaSyncedMoveCtrl::SetGunshipMoveTypeData(lua_State *L)
{
	CHoverAirMoveType* moveType = ParseMoveType<CHoverAirMoveType>(L, __FUNCTION__, 1);
	if (moveType == NULL) {
		luaL_error(L, "Unit does not have a compatible MoveType");
	}

	const int args = lua_gettop(L); // number of arguments

	if (args == 3 && lua_isstring(L, 2)) {
		// a single value
		SetSingleHoverAirMoveTypeValue(L, 2, 3, moveType);
	} else if (args == 2 && lua_istable(L, 2)) {
		// a table of values
		const int table = 2;
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (lua_israwstring(L, -2)) {
				SetSingleHoverAirMoveTypeValue(L, -2, -1, moveType);
			}
		}
	}

	return 0;
}

/******************************************************************************/
/******************************************************************************/

int LuaSyncedMoveCtrl::SetMoveDef(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	MoveDef* moveDef = NULL;

	if (unit == NULL) {
		lua_pushboolean(L, false);
		return 1;
	}
	if (unit->moveDef == NULL) {
		// aircraft or structure, not supported
		lua_pushboolean(L, false);
		return 1;
	}

	// MoveType instance must already have been assigned
	assert(unit->moveType != NULL);

	if (lua_isnumber(L, 2))
		moveDef = moveDefHandler->GetMoveDefByPathType(Clamp(luaL_checkint(L, 2), 0, int(moveDefHandler->GetNumMoveDefs()) - 1));
	if (lua_isstring(L, 2))
		moveDef = moveDefHandler->GetMoveDefByName(lua_tostring(L, 2));

	if (moveDef == NULL) {
		lua_pushboolean(L, false);
		return 1;
	}

	if (moveDef->unitDefRefCount == 0) {
		// pathfinder contains optimizations that
		// make unreferenced movedef's non-usable
		lua_pushboolean(L, false);
		return 1;
	}

	// the case where moveDef->pathType == unit->moveDef->pathType does no harm
	// note: if a unit (ID) is available, then its current MoveDef should always
	// be taken over the MoveDef corresponding to its UnitDef::pathType wherever
	// MoveDef properties are used in decision logic
	unit->moveDef = moveDef;

	lua_pushboolean(L, true);
	return 1;
}

