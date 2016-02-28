/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaSyncedMoveCtrl.h"

#include "LuaInclude.h"

#include "LuaHandle.h"
#include "LuaHashString.h"
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
#include "System/Sync/HsiehHash.h"

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
	CUnit* unit = unitHandler->GetUnit(luaL_checkint(L, index));

	if (unit == nullptr)
		return nullptr;
	if (!CanControlUnit(L, unit))
		return nullptr;

	return unit;
}


static inline CScriptMoveType* ParseScriptMoveType(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseUnit(L, caller, index);

	if (unit == nullptr)
		return nullptr;
	if (!unit->UsingScriptMoveType())
		return nullptr;

	return (static_cast<CScriptMoveType*>(unit->moveType));
}

template<typename DerivedMoveType>
static inline DerivedMoveType* ParseDerivedMoveType(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseUnit(L, caller, index);

	if (unit == nullptr)
		return nullptr;

	if (unit->moveType == nullptr)
		return nullptr;

	return (dynamic_cast<DerivedMoveType*>(unit->moveType));
}



/******************************************************************************/
/******************************************************************************/

int LuaSyncedMoveCtrl::IsEnabled(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);

	if (unit == NULL)
		return 0;

	lua_pushboolean(L, unit->UsingScriptMoveType());
	return 1;
}


int LuaSyncedMoveCtrl::Enable(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);

	if (unit == NULL)
		return 0;

	unit->EnableScriptMoveType();
	return 0;
}


int LuaSyncedMoveCtrl::Disable(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);

	if (unit == NULL)
		return 0;

	unit->DisableScriptMoveType();
	return 0;
}


/******************************************************************************/

int LuaSyncedMoveCtrl::SetTag(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __FUNCTION__, 1);

	if (moveType == NULL)
		return 0;

	moveType->tag = luaL_checkint(L, 2);
	return 0;
}


int LuaSyncedMoveCtrl::GetTag(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __FUNCTION__, 1);

	if (moveType == NULL)
		return 0;

	lua_pushnumber(L, moveType->tag);
	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaSyncedMoveCtrl::SetProgressState(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __FUNCTION__, 1);

	if (moveType == NULL)
		return 0;

	const int args = lua_gettop(L); // number of arguments
	if (args < 2) {
		luaL_error(L, "Incorrect arguments to SetProgressState()");
	}

	if (lua_isnumber(L, 2)) {
		const int state = lua_toint(L, 2);
		if ((state < AMoveType::Done) || (state > AMoveType::Failed)) {
			luaL_error(L, "SetProgressState(): bad state value (%d)", state);
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
	CScriptMoveType* moveType = ParseScriptMoveType(L, __FUNCTION__, 1);

	if (moveType == NULL)
		return 0;

	moveType->extrapolate = luaL_checkboolean(L, 2);
	return 0;
}


/******************************************************************************/

int LuaSyncedMoveCtrl::SetPhysics(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __FUNCTION__, 1);

	if (moveType == NULL)
		return 0;

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
	CScriptMoveType* moveType = ParseScriptMoveType(L, __FUNCTION__, 1);

	if (moveType == NULL)
		return 0;

	const float3 pos(luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3),
	                 luaL_checkfloat(L, 4));
	ASSERT_SYNCED(pos);
	moveType->SetPosition(pos);
	return 0;
}


int LuaSyncedMoveCtrl::SetVelocity(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __FUNCTION__, 1);

	if (moveType == NULL)
		return 0;

	const float3 vel(luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3),
	                 luaL_checkfloat(L, 4));
	ASSERT_SYNCED(vel);
	moveType->SetVelocity(vel);
	return 0;
}


int LuaSyncedMoveCtrl::SetRelativeVelocity(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __FUNCTION__, 1);

	if (moveType == NULL)
		return 0;

	const float3 relVel(luaL_checkfloat(L, 2),
	                    luaL_checkfloat(L, 3),
	                    luaL_checkfloat(L, 4));
	ASSERT_SYNCED(relVel);
	moveType->SetRelativeVelocity(relVel);
	return 0;
}


int LuaSyncedMoveCtrl::SetRotation(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __FUNCTION__, 1);

	if (moveType == NULL)
		return 0;

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
	CScriptMoveType* moveType = ParseScriptMoveType(L, __FUNCTION__, 1);

	if (moveType == NULL)
		return 0;

	const float3 rotVel(luaL_checkfloat(L, 2),
	                    luaL_checkfloat(L, 3),
	                    luaL_checkfloat(L, 4));
	ASSERT_SYNCED(rotVel);
	moveType->SetRotationVelocity(rotVel);
	return 0;
}


int LuaSyncedMoveCtrl::SetHeading(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __FUNCTION__, 1);

	if (moveType == NULL)
		return 0;

	const short heading = (short)luaL_checknumber(L, 2);
	ASSERT_SYNCED((short)heading);
	moveType->SetHeading(heading);
	return 0;
}


/******************************************************************************/

int LuaSyncedMoveCtrl::SetTrackSlope(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __FUNCTION__, 1);

	if (moveType == NULL)
		return 0;

	moveType->trackSlope = luaL_checkboolean(L, 2);
	return 0;
}


int LuaSyncedMoveCtrl::SetTrackGround(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __FUNCTION__, 1);

	if (moveType == NULL)
		return 0;

	moveType->trackGround = luaL_checkboolean(L, 2);
	return 0;
}


int LuaSyncedMoveCtrl::SetGroundOffset(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __FUNCTION__, 1);

	if (moveType == NULL)
		return 0;

	moveType->groundOffset = luaL_checkfloat(L, 2);
	return 0;
}


int LuaSyncedMoveCtrl::SetGravity(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __FUNCTION__, 1);

	if (moveType == NULL)
		return 0;

	moveType->gravityFactor = luaL_checkfloat(L, 2);
	return 0;
}


int LuaSyncedMoveCtrl::SetDrag(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __FUNCTION__, 1);

	if (moveType == NULL)
		return 0;

	moveType->drag = luaL_checkfloat(L, 2);
	return 0;
}


int LuaSyncedMoveCtrl::SetWindFactor(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __FUNCTION__, 1);

	if (moveType == NULL)
		return 0;

	moveType->windFactor = luaL_checkfloat(L, 2);
	return 0;
}


int LuaSyncedMoveCtrl::SetLimits(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __FUNCTION__, 1);

	if (moveType == NULL)
		return 0;

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
	CScriptMoveType* moveType = ParseScriptMoveType(L, __FUNCTION__, 1);

	if (moveType == NULL)
		return 0;

	// marks or unmarks the unit on the blocking-map, but
	// does not change its blocking (collidable) state
	moveType->SetNoBlocking(luaL_checkboolean(L, 2));
	return 0;
}


int LuaSyncedMoveCtrl::SetShotStop(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __FUNCTION__, 1);

	if (moveType == NULL)
		return 0;

	moveType->shotStop = luaL_checkboolean(L, 2);
	return 0;
}


int LuaSyncedMoveCtrl::SetSlopeStop(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __FUNCTION__, 1);

	if (moveType == NULL)
		return 0;

	moveType->slopeStop = luaL_checkboolean(L, 2);
	return 0;
}


int LuaSyncedMoveCtrl::SetCollideStop(lua_State* L)
{
	CScriptMoveType* moveType = ParseScriptMoveType(L, __FUNCTION__, 1);

	if (moveType == NULL)
		return 0;

	moveType->gndStop = luaL_checkboolean(L, 2); // FIXME
	moveType->collideStop = lua_toboolean(L, 2);
	return 0;
}



/******************************************************************************/
/* MoveType member-value handling */

template<typename value_type>
static bool SetMoveTypeValue(AMoveType* mt, const string& key, value_type value)
{
	return (mt->SetMemberValue(HsiehHash(key.c_str(), key.size(), 0), &value));
}

static inline bool SetMoveTypeValue(lua_State* L, AMoveType* moveType, int keyIdx, int valIdx)
{
	if (lua_isnumber(L, valIdx))
		return (SetMoveTypeValue<float>(moveType, lua_tostring(L, keyIdx), lua_tofloat(L, valIdx)));

	if (lua_isboolean(L, valIdx))
		return (SetMoveTypeValue<bool>(moveType, lua_tostring(L, keyIdx), lua_toboolean(L, valIdx)));

	return false;
}


static int SetMoveTypeData(lua_State* L, AMoveType* moveType, const char* caller)
{
	int numAssignedValues = 0;

	if (moveType == NULL) {
		luaL_error(L, "[%s] unit %d has incompatible movetype for %s", __FUNCTION__, lua_tonumber(L, 1), caller);
		return numAssignedValues;
	}

	switch (lua_gettop(L)) {
		case 2: {
			// two args (unitID, {"key1" = (number | bool) value1, ...})
			const int tableIdx = 2;

			if (lua_istable(L, tableIdx)) {
				for (lua_pushnil(L); lua_next(L, tableIdx) != 0; lua_pop(L, 1)) {
					if (lua_israwstring(L, -2) && SetMoveTypeValue(L, moveType, -2, -1)) {
						numAssignedValues++;
					} else {
						LOG_L(L_WARNING, "[%s] incompatible movetype key for %s", __FUNCTION__, caller);
					}
				}
			}
		} break;

		case 3: {
			// three args (unitID, "key", (number | bool) value)
			if (lua_isstring(L, 2) && SetMoveTypeValue(L, moveType, 2, 3)) {
				numAssignedValues++;
			} else {
				LOG_L(L_WARNING, "[%s] incompatible movetype key for %s", __FUNCTION__, caller);
			}
		} break;
	}

	lua_pushnumber(L, numAssignedValues);
	return 1;
}


int LuaSyncedMoveCtrl::SetGunshipMoveTypeData(lua_State* L)
{
	return (SetMoveTypeData(L, ParseDerivedMoveType<CHoverAirMoveType>(L, __FUNCTION__, 1), __FUNCTION__));
}

int LuaSyncedMoveCtrl::SetAirMoveTypeData(lua_State* L)
{
	return (SetMoveTypeData(L, ParseDerivedMoveType<CStrafeAirMoveType>(L, __FUNCTION__, 1), __FUNCTION__));
}

int LuaSyncedMoveCtrl::SetGroundMoveTypeData(lua_State* L)
{
	return (SetMoveTypeData(L, ParseDerivedMoveType<CGroundMoveType>(L, __FUNCTION__, 1), __FUNCTION__));
}



/******************************************************************************/
/******************************************************************************/

int LuaSyncedMoveCtrl::SetMoveDef(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	MoveDef* moveDef = nullptr;

	if (unit == nullptr) {
		lua_pushboolean(L, false);
		return 1;
	}
	if (unit->moveDef == nullptr) {
		// aircraft or structure, not supported
		lua_pushboolean(L, false);
		return 1;
	}

	// MoveType instance must already have been assigned
	assert(unit->moveType != nullptr);

	// parse a MoveDef by number *or* by string (mutually exclusive)
	if (lua_isnumber(L, 2))
		moveDef = moveDefHandler->GetMoveDefByPathType(Clamp(luaL_checkint(L, 2), 0, int(moveDefHandler->GetNumMoveDefs()) - 1));
	if (lua_isstring(L, 2))
		moveDef = moveDefHandler->GetMoveDefByName(lua_tostring(L, 2));

	if (moveDef == nullptr) {
		lua_pushboolean(L, false);
		return 1;
	}

	if (moveDef->udRefCount == 0) {
		// pathfinders contain optimizations that
		// make unreferenced movedef's non-usable
		LOG_L(L_ERROR, "SetMoveDef: Tried to use an unreferenced (:=disabled) MoveDef!");
		lua_pushboolean(L, false);
		return 1;
	}

	// PFS might have cached data by path-type which must be cleared
	if (unit->UsingScriptMoveType()) {
		unit->prevMoveType->StopMoving();
	} else {
		unit->moveType->StopMoving();
	}

	// the case where moveDef->pathType == unit->moveDef->pathType does no harm
	// note: if a unit (ID) is available, then its current MoveDef should always
	// be taken over the MoveDef corresponding to its UnitDef::pathType wherever
	// MoveDef properties are used in decision logic
	lua_pushboolean(L, (unit->moveDef = moveDef) != NULL);
	return 1;
}

