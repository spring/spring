/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#define LUA_SYNCED_ONLY

#include "LuaUnitScript.h"

#include "CobDefines.h"
#include "CobInstance.h"
#include "LuaInclude.h"
#include "NullUnitScript.h"
#include "UnitScriptFactory.h"
#include "LuaScriptNames.h"
#include "Lua/LuaConfig.h"
#include "Lua/LuaCallInCheck.h"
#include "Lua/LuaGaia.h"
#include "Lua/LuaHandleSynced.h"
#include "Lua/LuaRules.h"
#include "Lua/LuaUtils.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "System/ContainerUtil.h"
#include "System/SafeUtil.h"
#include "System/StringUtil.h"

CR_BIND_DERIVED(CLuaUnitScript, CUnitScript, )

CR_REG_METADATA(CLuaUnitScript, (
	CR_IGNORED(handle),
	CR_IGNORED(L),
	CR_MEMBER(scriptIndex),
	CR_MEMBER(scriptNames),
	CR_IGNORED(inKilled),
	CR_SERIALIZER(Serialize),
	CR_POSTLOAD(PostLoad)
))

void CLuaUnitScript::PostLoad()
{
	if (unit == nullptr)
		return;

	pieces.reserve(unit->localModel.pieces.size());

	for (auto& p: unit->localModel.pieces) {
		pieces.push_back(&p);
	}

	L = handle->GetLuaState();

	hasSetSFXOccupy  = scriptIndex[LUAFN_SetSFXOccupy ] != LUA_NOREF;
	hasRockUnit      = scriptIndex[LUAFN_RockUnit     ] != LUA_NOREF;
	hasStartBuilding = scriptIndex[LUAFN_StartBuilding] != LUA_NOREF;
}


void CLuaUnitScript::Serialize(creg::ISerializer* s) {
	bool isLuaGaia;
	if (s->IsWriting()) {
		isLuaGaia = luaGaia != nullptr && handle == &luaGaia->syncedLuaHandle;
		const bool isLuaRules = luaRules != nullptr && handle == &luaRules->syncedLuaHandle;
		assert(isLuaGaia || isLuaRules);
	}
	s->SerializeInt(&isLuaGaia, sizeof(isLuaGaia));
	if (!s->IsWriting()) {
		CSplitLuaHandle* slh = isLuaGaia ? (CSplitLuaHandle*) luaGaia : (CSplitLuaHandle*) luaRules;
		assert(slh != nullptr);
		handle = &slh->syncedLuaHandle;
		L = handle->GetLuaState();
	}
}


static inline LocalModelPiece* ParseLocalModelPiece(lua_State* L, CUnitScript* script, const char* caller)
{
	const int piece = luaL_checkint(L, 1) - 1;

	if (!script->PieceExists(piece))
		luaL_error(L, "%s(): Invalid piecenumber", caller);

	return (script->GetScriptLocalModelPiece(piece));
}

static inline int ToLua(lua_State* L, const float3& v)
{
	lua_pushnumber(L, v.x);
	lua_pushnumber(L, v.y);
	lua_pushnumber(L, v.z);
	return 3;
}

/*

some notes:
- all piece numbers are 1 based, to be consistent with other parts of interface
- all axes are 1 based, so 1 = x, 2 = y, 3 = z
- destination, speed for Move are in world coords, and NOT in COB coords
- therefore, compared to COB, the X axis for the Move callout is mirrored
- destination, speed, accel, decel for Turn, Spin, StopSpin are in radians
- GetUnitCOBValue(PLAY_SOUND, ...) does NOT work for Lua unit scripts,
  use Spring.PlaySound instead (synced code can call unsynced funcs!).
- Because in current design CBCobThreadFinish can impossibly be called, certain
  state changes which normally happen immediately when script returns should
  be triggered through a call to a callOut when using Lua scripts.
  This applies to:
  * Spring.SetUnitShieldState(unitID, false|true) replaces return value 0|1 of
    COB's AimWeaponX function for plasma repulsers.
  * Spring.SetUnitWeaponState(unitID, weaponNum, "aimReady", 0|1) replaces
    return value 0|1 of COB's AimWeaponX function for all other weapons.
  * Spring.UnitScript.SetDeathScriptFinished(wreckLevel) replaces
    return value of wreckLevel from Killed function.
    This MUST be called, otherwise zombie units will eat your Spring!


callIn notes:
- Killed takes recentDamage,maxHealth instead of severity, to calculate
  severity use: 'local severity = recentDamage / maxHealth'
- SetMaxReloadTime doesn't exist, max reload time can be calculated in Lua
  (without the WTFs that are present in max reload time calculation for COB)
- RockUnit takes x,z instead of 500z,500x
- HitByWeapon takes x,z instead of 500z,500x
- HitByWeaponId takes x,z,weaponDefID,damage instead of 500z,500x,tdfID,damage
- HitByWeaponId returns the new damage, instead of a percentage of old damage
- QueryLandingPadCount doesn't exist
- QueryLandingPad should return an array (table) of all pieces
- BeginTransport and QueryTransport take unitID instead of unit->height*65536,
  use 'local height = Spring.GetUnitHeight(unitID)' to get the height.
- TransportDrop takes x,y,z instead of PACKXZ(x,z)
- AimWeapon for a shield (plasma repulser) takes no arguments instead of 0,0
- Shot takes no arguments instead of 0
- new callins MoveFinished and TurnFinished, see below


docs for callins defined in this file:

  TODO: document other callins properly

TurnFinished(number piece, number axis)
	Called after a turn finished for this unit/piece/axis (not a turn-now!)
	Should resume coroutine of the particular thread which called the Lua
	WaitForTurn function (see below).

MoveFinished(number piece, number axis)
	Called after a move finished for this unit/piece/axis (not a move-now!)
	Should resume coroutine of the particular thread which called the Lua
	WaitForMove function (see below).


docs for callouts defined in this file:

Spring.UnitScript.SetUnitValue(...)
	see wiki for Spring.SetUnitCOBValue (unchanged)

Spring.UnitScript.GetUnitValue(...)
	see wiki for Spring.GetUnitCOBValue (unchanged)

Spring.UnitScript.SetPieceVisibility(number piece, boolean visible) -> nil
	Set's piece visibility.  Same as COB's hide/show.

Spring.UnitSript.EmitSfx(number piece, number type) -> nil
	Same as COB's emit-sfx.

Spring.UnitScript.AttachUnit(number piece, number transporteeID) -> nil
	Same as COB's attach-unit.

Spring.UnitScript.DropUnit(number transporteeID) -> nil
	Same as COB's drop-unit.

Spring.UnitScript.Explode(number piece, number flags) -> nil
	Same as COB's explode.

Spring.UnitScript.ShowFlare(number piece) -> nil
	Same as COB's show _inside_ FireWeaponX.

Spring.UnitScript.Spin(number piece, number axis, number speed[, number accel]) -> nil
	Same as COB's spin.  If accel isn't given spinning starts at the desired speed.

Spring.UnitScript.StopSpin(number piece, number axis[, number decel]) -> nil
	Same as COB's stop-spin.  If decel isn't given spinning stops immediately.

Spring.UnitScript.Turn(number piece, number axis, number destination[, number speed]) -> nil
	Same as COB's turn iff speed is given and not zero, and turn-now otherwise.

Spring.UnitScript.Move(number piece, number axis, number destination[, number speed]) -> nil
	Same as COB's move iff speed is given and not zero, and move-now otherwise.

Spring.UnitScript.IsInTurn(number piece, number axis) -> boolean
Spring.UnitScript.IsInMove(number piece, number axis) -> boolean
Spring.UnitScript.IsInSpin(number piece, number axis) -> boolean
	Returns true iff such an animation exists, false otherwise.

Spring.UnitScript.WaitForTurn(number piece, number axis) -> boolean
	Returns true iff such an animation exists, false otherwise.  Iff it returns
	true, the TurnFinished callIn will be called once the turn completes.

Spring.UnitScript.WaitForMove(number piece, number axis) -> boolean
	Returns true iff such an animation exists, false otherwise.  Iff it returns
	true, the MoveFinished callIn will be called once the move completes.

Spring.UnitScript.SetDeathScriptFinished(number wreckLevel])
	Tells Spring the Killed script finished, and which wreckLevel to use.
	If wreckLevel is not given no wreck is created.

Spring.UnitScript.CreateScript(number unitID, table callIns) -> nil
	Replaces the current unit script (independent of type, also replaces COB)
	with the unit script given by a table of callins for the unit.
	Callins are similar to COB functions, e.g. a number of predefined names are
	called by the engine if they exist in the table.

Spring.UnitScript.UpdateCallIn(number unitID, string fname[, function callIn]) -> number|boolean
	Iff callIn is a function, a single callIn is replaced or added, and the
	new functionID is returned.  If callIn isn't given or is nil, the callIn is
	nilled, returns true if it was removed, or false if the callin didn't exist.
	See also Spring.UnitScript.CreateScript.
*/


#define LUA_TRACE(m) \
	do { \
		if (unit) { \
			LOG_L(L_DEBUG, "%s: %d: %s", __func__, unit->id, m); \
		} else { \
			LOG_L(L_DEBUG, "%s: %s", __func__, m); \
		} \
	} while (false)


CUnit* CLuaUnitScript::activeUnit;
CUnitScript* CLuaUnitScript::activeScript;


/******************************************************************************/
/******************************************************************************/


CLuaUnitScript::CLuaUnitScript(lua_State* L, CUnit* unit)
	: CUnitScript(unit)
	, handle(CLuaHandle::GetHandle(L)), L(L)
{
	scriptIndex.fill(LUA_NOREF);
	scriptNames.reserve(scriptIndex.size());
	pieces.reserve(unit->localModel.pieces.size());

	for (lua_pushnil(L); lua_next(L, 2) != 0; /*lua_pop(L, 1)*/) {
		const std::string& fname = lua_tostring(L, -2);
		const int r = luaL_ref(L, LUA_REGISTRYINDEX);

		scriptNames.insert(fname, r);
		UpdateCallIn(fname, r);
	}
	for (auto& p: unit->localModel.pieces) {
		pieces.push_back(&p);
	}
}


CLuaUnitScript::~CLuaUnitScript()
{
	// if L is NULL then the lua_State is closed/closing (see HandleFreed)
	if (L == nullptr)
		return;

	// notify Lua the script is going down
	Destroy();

	for (auto it = scriptNames.begin(); it != scriptNames.end(); ++it) {
		luaL_unref(L, LUA_REGISTRYINDEX, it->second);
	}
}


void CLuaUnitScript::HandleFreed(CLuaHandle* handle)
{
	for (CUnit* u: unitHandler.GetActiveUnits()) {
		CUnitScript* script = u->script;
		CLuaUnitScript* luaScript = dynamic_cast<CLuaUnitScript*>(script);

		// kill only the Lua scripts running in this handle
		if (luaScript == nullptr)
			continue;
		if (luaScript->handle != handle)
			continue;

		// we don't have anything better ...
		u->script = &CNullUnitScript::value;

		// signal the destructor it shouldn't unref refs
		luaScript->L = nullptr;

		spring::SafeDestruct(script);
	}
}


int CLuaUnitScript::UpdateCallIn()
{
	const std::string& fname = lua_tostring(L, 2);
	const bool remove = lua_isnoneornil(L, 3);

	auto it = scriptNames.find(fname);
	int r = LUA_NOREF;

	if (it != scriptNames.end()) {
		luaL_unref(L, LUA_REGISTRYINDEX, it->second);
		if (remove) {
			// removing existing callIn
			scriptNames.erase(it);
			lua_pushboolean(L, 1);
		}
		else {
			// replacing existing callIn
			r = luaL_ref(L, LUA_REGISTRYINDEX);
			it->second = r;
		}
	}
	else if (remove) {
		// removing nonexisting callIn (== no-op)
		lua_pushboolean(L, 0);
	}
	else {
		// adding new callIn
		r = luaL_ref(L, LUA_REGISTRYINDEX);
		scriptNames.insert(fname, r);
	}

	UpdateCallIn(fname, r);

	if (!remove) {
		// the reference doubles as the functionId, as expected by RealCall
		// from Lua this can be used with e.g. Spring.CallCOBScript
		lua_pushnumber(L, r);
	}
	return 1;
}


void CLuaUnitScript::UpdateCallIn(const std::string& fname, int ref)
{
	// Map common function names to indices
	int num = CLuaUnitScriptNames::GetScriptNumber(fname);

	// Check upper bound too in case user calls UpdateCallIn with nonexisting weapon.
	// (we only allocate slots in scriptIndex for the number of weapons the unit has)
	if (num >= 0 && num < int(scriptIndex.size())) {
		scriptIndex[num] = ref;
	}

	switch (num) {
		case LUAFN_SetSFXOccupy:  hasSetSFXOccupy  = (ref != LUA_NOREF); break;
		case LUAFN_RockUnit:      hasRockUnit      = (ref != LUA_NOREF); break;
		case LUAFN_StartBuilding: hasStartBuilding = (ref != LUA_NOREF); break;
	}

	//LUA_TRACE(fname.c_str());
}


void CLuaUnitScript::RemoveCallIn(const std::string& fname)
{
	const auto it = scriptNames.find(fname);

	if (it != scriptNames.end()) {
		luaL_unref(L, LUA_REGISTRYINDEX, it->second);
		scriptNames.erase(it);
		UpdateCallIn(fname, LUA_NOREF);
	}

	//LUA_TRACE(fname.c_str());
}


void CLuaUnitScript::ShowScriptError(const std::string& msg)
{
	// if we are in the same handle, we can truely raise an error
	if (handle->IsRunning()) {
		luaL_error(L, "Lua UnitScript error: %s", msg.c_str());
	}
	else {
		LOG_L(L_ERROR, "%s", msg.c_str());
	}
}


bool CLuaUnitScript::HasBlockShot(int weaponNum) const
{
	return HasFunction(LUAFN_BlockShot);
}


bool CLuaUnitScript::HasTargetWeight(int weaponNum) const
{
	return HasFunction(LUAFN_TargetWeight);
}


/******************************************************************************/
/******************************************************************************/


inline float CLuaUnitScript::PopNumber(int fn, float def)
{
	if (!lua_israwnumber(L, -1)) {
		const std::string& fname = CLuaUnitScriptNames::GetScriptName(fn);

		LOG_L(L_ERROR, "%s: bad return value, expected number", fname.c_str());
		RemoveCallIn(fname);

		lua_pop(L, 1);
		return def;
	}

	const float ret = lua_tonumber(L, -1);
	lua_pop(L, 1);
	return ret;
}


inline bool CLuaUnitScript::PopBoolean(int fn, bool def)
{
	if (!lua_isboolean(L, -1)) {
		const std::string& fname = CLuaUnitScriptNames::GetScriptName(fn);

		LOG_L(L_ERROR, "%s: bad return value, expected boolean", fname.c_str());
		RemoveCallIn(fname);

		lua_pop(L, 1);
		return def;
	}

	const bool ret = lua_toboolean(L, -1);
	lua_pop(L, 1);
	return ret;
}


inline void CLuaUnitScript::RawPushFunction(int functionId)
{
	// Push Lua function on the stack
	lua_rawgeti(L, LUA_REGISTRYINDEX, functionId);
}


inline void CLuaUnitScript::PushFunction(int id)
{
	RawPushFunction(scriptIndex[id]);
}


inline void CLuaUnitScript::PushUnit(const CUnit* targetUnit)
{
	if (targetUnit) {
		lua_pushnumber(L, targetUnit->id);
	}
	else {
		lua_pushnil(L);
	}
}


inline bool CLuaUnitScript::RunCallIn(int id, int inArgs, int outArgs)
{
	return RawRunCallIn(scriptIndex[id], inArgs, outArgs);
}


int CLuaUnitScript::RunQueryCallIn(int fn)
{
	if (!HasFunction(fn))
		return -1;

	LUA_CALL_IN_CHECK(L, -1);
	lua_checkstack(L, 1);

	PushFunction(fn);

	if (!RunCallIn(fn, 0, 1))
		return -1;

	const int scriptPieceNum = (int)PopNumber(fn, 0) - 1;

	// if (LOG_IS_ENABLED(L_DEBUG)) {
		// if (PieceExists(scriptPieceNum)) {
			// LocalModelPiece* piece = GetScriptLocalModelPiece(scriptPieceNum);
			// LOG_L(L_DEBUG, "%s: %d %s",
					// CLuaUnitScriptNames::GetScriptName(fn).c_str(),
					// scriptPieceNum,
					// (piece->original) ? piece->original->name.c_str() : "n/a");
		// }
	// }

	return scriptPieceNum;
}


int CLuaUnitScript::RunQueryCallIn(int fn, float arg1)
{
	if (!HasFunction(fn))
		return -1;

	LUA_CALL_IN_CHECK(L, -1);
	lua_checkstack(L, 2);

	PushFunction(fn);
	lua_pushnumber(L, arg1);

	if (!RunCallIn(fn, 1, 1))
		return -1;

	const int scriptPieceNum = (int)PopNumber(fn, 0) - 1;

	// if (LOG_IS_ENABLED(L_DEBUG)) {
		// if (PieceExists(scriptPieceNum)) {
			// LocalModelPiece* piece = GetScriptLocalModelPiece(scriptPieceNum);
			// LOG_L(L_DEBUG, "%s: %d %s",
					// CLuaUnitScriptNames::GetScriptName(fn).c_str(),
					// scriptPieceNum,
					// (piece->original) ? piece->original->name.c_str() : "n/a");
		// }
	// }

	return scriptPieceNum;
}


void CLuaUnitScript::Call(int fn, float arg1)
{
	if (!HasFunction(fn))
		return;

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);

	PushFunction(fn);
	lua_pushnumber(L, arg1);

	RunCallIn(fn, 1, 0);
}


void CLuaUnitScript::Call(int fn, float arg1, float arg2)
{
	if (!HasFunction(fn))
		return;

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);

	PushFunction(fn);
	lua_pushnumber(L, arg1);
	lua_pushnumber(L, arg2);

	RunCallIn(fn, 2, 0);
}


void CLuaUnitScript::Call(int fn, float arg1, float arg2, float arg3)
{
	if (!HasFunction(fn))
		return;

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);

	PushFunction(fn);
	lua_pushnumber(L, arg1);
	lua_pushnumber(L, arg2);
	lua_pushnumber(L, arg3);

	RunCallIn(fn, 3, 0);
}


/******************************************************************************/
/******************************************************************************/


void CLuaUnitScript::Create()
{
	// There is no use for Create
	// (Lua code can just call it after Spring.UnitScript.CreateScript(...))
}


void CLuaUnitScript::Killed()
{
	const int fn = LUAFN_Killed;

	if (!HasFunction(fn)) {
		unit->KilledScriptFinished(unit->delayedWreckLevel);
		return;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);

	PushFunction(fn);
	lua_pushnumber(L, unit->recentDamage);
	lua_pushnumber(L, unit->maxHealth);

	inKilled = true;

	if (!RunCallIn(fn, 2, 1))
		return;

	// If Killed returns an integer, it signals it hasn't started a thread.
	// In this case the return value is the delayedWreckLevel.
	if (lua_israwnumber(L, -1)) {
		inKilled = false;
		unit->KilledScriptFinished(lua_toint(L, -1));
	}
	else if (!lua_isnoneornil(L, -1)) {
		const std::string& fname = CLuaUnitScriptNames::GetScriptName(fn);

		LOG_L(L_ERROR, "%s: bad return value, expected number or nil", fname.c_str());
		RemoveCallIn(fname);

		// without this we would end up with zombie units
		unit->KilledScriptFinished(unit->delayedWreckLevel);
	}

	lua_pop(L, 1);
}


void CLuaUnitScript::WindChanged(float heading, float speed)
{
	Call(LUAFN_WindChanged, heading, speed);
}


void CLuaUnitScript::ExtractionRateChanged(float speed)
{
	Call(LUAFN_ExtractionRateChanged, speed);
}



void CLuaUnitScript::WorldRockUnit(const float3& rockDir) { RockUnit(unit->GetObjectSpaceVec(rockDir)); }
void CLuaUnitScript::RockUnit(const float3& rockDir)
{
	Call(LUAFN_RockUnit, rockDir.x, rockDir.z);
}

void CLuaUnitScript::WorldHitByWeapon(const float3& hitDir, int weaponDefId, float& inoutDamage) { HitByWeapon(unit->GetObjectSpaceVec(hitDir), weaponDefId, inoutDamage); }
void CLuaUnitScript::HitByWeapon(const float3& hitDir, int weaponDefId, float& inoutDamage)
{
	const int fn = LUAFN_HitByWeapon;

	if (!HasFunction(fn))
		return;

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);

	PushFunction(fn);
	lua_pushnumber(L, hitDir.x);
	lua_pushnumber(L, hitDir.z);
	lua_pushnumber(L, weaponDefId);
	lua_pushnumber(L, inoutDamage);

	if (!RunCallIn(fn, 4, 1))
		return;

	if (lua_israwnumber(L, -1)) {
		inoutDamage = lua_tonumber(L, -1);
	}
	else if (!lua_isnoneornil(L, -1)) {
		const std::string& fname = CLuaUnitScriptNames::GetScriptName(fn);

		LOG_L(L_ERROR, "%s: bad return value, expected number or nil", fname.c_str());
		RemoveCallIn(fname);
	}

	lua_pop(L, 1);
}


void CLuaUnitScript::SetSFXOccupy(int curTerrainType)
{
	const int fn = LUAFN_SetSFXOccupy;

	if (!HasFunction(fn))
		return;

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);

	PushFunction(fn);
	lua_pushnumber(L, curTerrainType);

	RunCallIn(fn, 1, 0);
}


void CLuaUnitScript::QueryLandingPads(std::vector<int>& out_pieces)
{
	const int fn = LUAFN_QueryLandingPads;

	if (!HasFunction(fn))
		return;

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);

	PushFunction(fn);

	if (!RunCallIn(fn, 0, 1))
		return;

	if (lua_istable(L, -1)) {
		int n = 1;
		// get the first piece number at t[n=1]
		lua_rawgeti(L, -1, n);

		// t = {[1] = piece_number_1, [2] = piece_number_2, ...}
		while (lua_israwnumber(L, -1)) {
			out_pieces.push_back(lua_toint(L, -1) - 1);
			lua_pop(L, 1);
			lua_rawgeti(L, -1, ++n);
		}

		lua_pop(L, 1);
	} else {
		const std::string& fname = CLuaUnitScriptNames::GetScriptName(fn);

		LOG_L(L_ERROR, "%s: bad return value, expected table", fname.c_str());
		RemoveCallIn(fname);
	}

	lua_pop(L, 1);
}


void CLuaUnitScript::BeginTransport(const CUnit* unit)
{
	Call(LUAFN_BeginTransport, unit->id);
}


int CLuaUnitScript::QueryTransport(const CUnit* unit)
{
	return RunQueryCallIn(LUAFN_QueryTransport, unit->id);
}


void CLuaUnitScript::TransportPickup(const CUnit* unit)
{
	Call(LUAFN_TransportPickup, unit->id);
}


void CLuaUnitScript::TransportDrop(const CUnit* unit, const float3& pos)
{
	const int fn = LUAFN_TransportDrop;

	if (!HasFunction(fn))
		return;

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);

	PushFunction(fn);
	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);

	RunCallIn(fn, 4, 0);
}


void CLuaUnitScript::StartBuilding(float heading, float pitch)
{
	Call(LUAFN_StartBuilding, heading, pitch);
}


int CLuaUnitScript::QueryNanoPiece()
{
	return RunQueryCallIn(LUAFN_QueryNanoPiece);
}


int CLuaUnitScript::QueryBuildInfo()
{
	return RunQueryCallIn(LUAFN_QueryBuildInfo);
}


int CLuaUnitScript::QueryWeapon(int weaponNum)
{
	return RunQueryCallIn(LUAFN_QueryWeapon, weaponNum + LUA_WEAPON_BASE_INDEX);
}


void CLuaUnitScript::AimWeapon(int weaponNum, float heading, float pitch)
{
	Call(LUAFN_AimWeapon, weaponNum + LUA_WEAPON_BASE_INDEX, heading, pitch);
}


void  CLuaUnitScript::AimShieldWeapon(CPlasmaRepulser* weapon)
{
	Call(LUAFN_AimShield, weapon->weaponNum + LUA_WEAPON_BASE_INDEX);
}


int CLuaUnitScript::AimFromWeapon(int weaponNum)
{
	return RunQueryCallIn(LUAFN_AimFromWeapon, weaponNum + LUA_WEAPON_BASE_INDEX);
}


void CLuaUnitScript::Shot(int weaponNum)
{
	// FIXME: pass projectileID?
	Call(LUAFN_Shot, weaponNum + LUA_WEAPON_BASE_INDEX);
}


bool CLuaUnitScript::BlockShot(int weaponNum, const CUnit* targetUnit, bool userTarget)
{
	const int fn = LUAFN_BlockShot;

	if (!HasFunction(fn))
		return false;

	LUA_CALL_IN_CHECK(L, false);
	lua_checkstack(L, 4);

	PushFunction(fn);
	lua_pushnumber(L, weaponNum + LUA_WEAPON_BASE_INDEX);
	PushUnit(targetUnit);
	lua_pushboolean(L, userTarget);

	if (!RunCallIn(fn, 3, 1))
		return false;

	return PopBoolean(fn, false);
}


float CLuaUnitScript::TargetWeight(int weaponNum, const CUnit* targetUnit)
{
	const int fn = LUAFN_TargetWeight;

	if (!HasFunction(fn))
		return 1.0f;

	LUA_CALL_IN_CHECK(L, 1.0f);
	lua_checkstack(L, 3);

	PushFunction(fn);
	lua_pushnumber(L, weaponNum + LUA_WEAPON_BASE_INDEX);
	PushUnit(targetUnit);

	if (!RunCallIn(fn, 2, 1))
		return 1.0f;

	return PopNumber(fn, 1.0f);
}


void CLuaUnitScript::AnimFinished(AnimType type, int piece, int axis)
{
	Call((type == AMove)? LUAFN_MoveFinished : LUAFN_TurnFinished, piece + 1, axis + 1);
}


void CLuaUnitScript::RawCall(int functionId)
{
	if (functionId < 0)
		return;

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 1);

	RawPushFunction(functionId);
	RawRunCallIn(functionId, 0, 0);
}


std::string CLuaUnitScript::GetScriptName(int functionId) const
{
	// only for error messages, so speed doesn't matter
	const auto pred = [functionId](const decltype(scriptNames)::value_type& p) { return (functionId == p.second); };
	const auto iter = std::find_if(scriptNames.begin(), scriptNames.end(), pred);

	if (iter != scriptNames.end())
		return iter->first;

	return ("<unnamed: " + IntToString(functionId) + ">");
}


bool CLuaUnitScript::RawRunCallIn(int functionId, int inArgs, int outArgs)
{
	CUnit* oldActiveUnit = activeUnit;
	CUnitScript* oldActiveScript = activeScript;

	activeUnit = unit;
	activeScript = this;

	std::string err;
	const int error = handle->RunCallInLUS(L, &err, inArgs, outArgs);

	activeUnit = oldActiveUnit;
	activeScript = oldActiveScript;

	if (error == 0)
		return true;

	const std::string& hname = handle->GetName();
	const std::string& fname = GetScriptName(functionId);

	LOG_L(L_ERROR, "[LuaUnitScript::%s][%s::%s] error=%i trace=%s", __func__, hname.c_str(), fname.c_str(), error, err.c_str());
	RemoveCallIn(fname);

	return false;
}


void CLuaUnitScript::Destroy() { Call(LUAFN_Destroy); }
void CLuaUnitScript::StartMoving(bool reversing) { Call(LUAFN_StartMoving, reversing * 1.0f); }
void CLuaUnitScript::StopMoving() { Call(LUAFN_StopMoving); }
void CLuaUnitScript::StartSkidding(const float3& vel) { Call(LUAFN_StartSkidding, vel.x, vel.y, vel.z); }
void CLuaUnitScript::StopSkidding() { Call(LUAFN_StopSkidding); }
void CLuaUnitScript::ChangeHeading(short deltaHeading) { Call(LUAFN_ChangeHeading, deltaHeading * 1.0f); }
void CLuaUnitScript::StartUnload() { Call(LUAFN_StartUnload); }
void CLuaUnitScript::EndTransport() { Call(LUAFN_EndTransport); }
void CLuaUnitScript::StartBuilding() { Call(LUAFN_StartBuilding); }
void CLuaUnitScript::StopBuilding() { Call(LUAFN_StopBuilding); }
void CLuaUnitScript::Falling() { Call(LUAFN_Falling); }
void CLuaUnitScript::Landed() { Call(LUAFN_Landed); }
void CLuaUnitScript::Activate() { Call(LUAFN_Activate); }
void CLuaUnitScript::Deactivate() { Call(LUAFN_Deactivate); }
void CLuaUnitScript::MoveRate(int curRate) { Call(LUAFN_MoveRate, curRate); }
void CLuaUnitScript::FireWeapon(int weaponNum) { Call(LUAFN_FireWeapon, weaponNum + LUA_WEAPON_BASE_INDEX); }
void CLuaUnitScript::EndBurst(int weaponNum) { Call(LUAFN_EndBurst, weaponNum + LUA_WEAPON_BASE_INDEX); }


/******************************************************************************/
/******************************************************************************/


bool CLuaUnitScript::PushEntries(lua_State* L)
{
	{
		// reset these in case we were reloaded
		activeUnit = nullptr;
		activeScript = nullptr;
	}

	lua_pushstring(L, "UnitScript");
	lua_newtable(L);

	REGISTER_LUA_CFUNC(CreateScript);
	REGISTER_LUA_CFUNC(UpdateCallIn);
	REGISTER_LUA_CFUNC(CallAsUnit);

	REGISTER_LUA_CFUNC(GetUnitValue);
	REGISTER_LUA_CFUNC(SetUnitValue);
	REGISTER_LUA_CFUNC(SetPieceVisibility);
	REGISTER_LUA_CFUNC(EmitSfx);
	REGISTER_LUA_CFUNC(AttachUnit);
	REGISTER_LUA_CFUNC(DropUnit);
	REGISTER_LUA_CFUNC(Explode);
	REGISTER_LUA_CFUNC(ShowFlare);

	REGISTER_LUA_CFUNC(Spin);
	REGISTER_LUA_CFUNC(StopSpin);
	REGISTER_LUA_CFUNC(Turn);
	REGISTER_LUA_CFUNC(Move);
	REGISTER_LUA_CFUNC(IsInTurn);
	REGISTER_LUA_CFUNC(IsInMove);
	REGISTER_LUA_CFUNC(IsInSpin);
	REGISTER_LUA_CFUNC(WaitForTurn);
	REGISTER_LUA_CFUNC(WaitForMove);

	REGISTER_LUA_CFUNC(SetDeathScriptFinished);

	REGISTER_LUA_CFUNC(GetPieceTranslation);
	REGISTER_LUA_CFUNC(GetPieceRotation);
	REGISTER_LUA_CFUNC(GetPiecePosDir);

	REGISTER_LUA_CFUNC(GetActiveUnitID);

	lua_rawset(L, -3);

	// backwards compatibility
	REGISTER_LUA_CFUNC(GetUnitCOBValue);
	REGISTER_LUA_CFUNC(SetUnitCOBValue);

	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Parsing helpers
//

// FIXME: this badly needs a clean up, it's duplicated

static inline CUnit* ParseRawUnit(lua_State* L, const char* caller, int index)
{
	if (!lua_israwnumber(L, index))
		luaL_error(L, "%s(): Bad unitID", caller);

	CUnit* u = unitHandler.GetUnit(lua_toint(L, index));

	if (u == nullptr)
		luaL_error(L, "%s(): Bad unitID: %d", caller, lua_toint(L, index));

	return u;
}


static inline CUnit* ParseUnit(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseRawUnit(L, caller, index);

	if (unit == nullptr)
		return nullptr;

	if (!CanControlUnit(L, unit))
		return nullptr;

	return unit;
}


static inline int ParseAxis(lua_State* L, const char* caller, int index)
{
	if (!lua_israwnumber(L, index))
		luaL_error(L, "%s(): Bad axis", caller);

	const int axis  = lua_toint(L, index) - 1;

	if ((axis < 0) || (axis > 2))
		luaL_error(L, "%s(): Bad axis", caller);

	return axis;
}


/******************************************************************************/
/******************************************************************************/


int CLuaUnitScript::CreateScript(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	// check table of callIns
	// (we might not get a chance to clean up later on, if something is wrong)
	if (!lua_istable(L, 2))
		luaL_error(L, "%s(): error parsing callIn table", __func__);

	for (lua_pushnil(L); lua_next(L, 2) != 0; lua_pop(L, 1)) {
		if (!lua_israwstring(L, -2) || !lua_isfunction(L, -1)) {
			luaL_error(L, "%s(): error parsing callIn table", __func__);
		}
	}

	if (unit->script != &CNullUnitScript::value)
		spring::SafeDestruct(unit->script);

	// replace the unit's script (ctor parses callIn table)
	unit->script = CUnitScriptFactory::CreateLuaScript(unit, L);
	return 0;
}


int CLuaUnitScript::UpdateCallIn(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	CLuaUnitScript* script = dynamic_cast<CLuaUnitScript*>(unit->script);

	if (script == nullptr)
		luaL_error(L, "%s(): not a Lua unit script", __func__);

	// we would get confused if our refs aren't together in a single state
	if (L != script->L)
		luaL_error(L, "%s(): incorrect lua_State", __func__);

	if (!lua_israwstring(L, 2) || (!lua_isfunction(L, 3) && !lua_isnoneornil(L, 3)))
		luaL_error(L, "Incorrect arguments to %s()", __func__);

	return script->UpdateCallIn();
}


int CLuaUnitScript::CallAsUnit(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const int funcIndex = 2;

	if (!lua_isfunction(L, funcIndex))
		luaL_error(L, "Incorrect arguments to %s()", __func__);

	CUnit* oldActiveUnit = activeUnit;
	CUnitScript* oldActiveScript = activeScript;

	activeUnit = unit;
	activeScript = unit->script;

	const int error = lua_pcall(L, lua_gettop(L) - funcIndex, LUA_MULTRET, 0);

	activeUnit = oldActiveUnit;
	activeScript = oldActiveScript;

	if (error != 0)
		lua_error(L);

	return (lua_gettop(L) - funcIndex + 1);
}


// moved from LuaSyncedCtrl

int CLuaUnitScript::GetUnitValue(lua_State* L, CUnitScript* script, int arg)
{
	bool splitData = false;
	if (lua_isboolean(L, arg)) {
		splitData = lua_toboolean(L, arg);
		arg++;
	}

	const int val = luaL_checkint(L, arg); arg++;

	int p[4];
	for (int a = 0; a < 4; a++, arg++) {
		if (lua_istable(L, arg)) {
			int x, z;
			lua_rawgeti(L, arg, 1); x = luaL_checkint(L, -1); lua_pop(L, 1);
			lua_rawgeti(L, arg, 2); z = luaL_checkint(L, -1); lua_pop(L, 1);
			p[a] = PACKXZ(x, z);
		}
		else {
			p[a] = (int)luaL_optnumber(L, arg, 0);
		}
	}

	const int result = script->GetUnitVal(val, p[0], p[1], p[2], p[3]);
	if (!splitData) {
		lua_pushnumber(L, result);
		return 1;
	}
	lua_pushnumber(L, UNPACKX(result));
	lua_pushnumber(L, UNPACKZ(result));
	return 2;
}


int CLuaUnitScript::GetUnitCOBValue(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	return GetUnitValue(L, unit->script, 2);
}


int CLuaUnitScript::GetUnitValue(lua_State* L)
{
	if (activeScript == nullptr)
		return 0;

	return GetUnitValue(L, activeScript, 1);
}


// moved from LuaSyncedCtrl

int CLuaUnitScript::SetUnitValue(lua_State* L, CUnitScript* script, int arg)
{
	const int args = lua_gettop(L) - arg; // number of arguments
	const int val = luaL_checkint(L, arg++);
	int param;
	if (args == 1) {
		param = lua_isboolean(L, arg) ? int(lua_toboolean(L, arg++)) : luaL_checkint(L, arg++);
	}
	else {
		const int x = luaL_checkint(L, arg++);
		const int z = luaL_checkint(L, arg++);
		param = PACKXZ(x, z);
	}
	script->SetUnitVal(val, param);
	return 0;
}


int CLuaUnitScript::SetUnitCOBValue(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	return SetUnitValue(L, unit->script, 2);
}


int CLuaUnitScript::SetUnitValue(lua_State* L)
{
	if (activeScript == nullptr)
		return 0;

	return SetUnitValue(L, activeScript, 1);
}


int CLuaUnitScript::SetPieceVisibility(lua_State* L)
{
	// void SetVisibility(int piece, bool visible);
	if (activeScript == nullptr)
		return 0;

	// note: for Lua unit scripts it would be confusing if the unit's
	// unit->script->pieces differs from the unit->localModel->pieces.

	const int piece = luaL_checkint(L, 1) - 1;
	const bool visible = lua_toboolean(L, 2);
	activeScript->SetVisibility(piece, visible);
	return 0;
}


int CLuaUnitScript::EmitSfx(lua_State* L)
{
	// void EmitSfx(int type, int piece);
	if (activeScript == nullptr)
		return 0;

	// note: the arguments are reversed compared to the C++ (and COB?) function
	const int piece = luaL_checkint(L, 1) - 1;
	const int type = lua_isnumber(L, 2)? luaL_checkint(L, 2): (explGenHandler.LoadCustomGeneratorID(lua_tostring(L, 2)) | SFX_GLOBAL);

	activeScript->EmitSfx(type, piece);
	return 0;
}


int CLuaUnitScript::AttachUnit(lua_State* L)
{
	// void AttachUnit(int piece, int unit);
	if (activeScript == nullptr)
		return 0;

	const CUnit* transportee = ParseUnit(L, __func__, 2);

	if (transportee == nullptr)
		return 0;

	activeScript->AttachUnit(luaL_checkint(L, 1) - 1, transportee->id);
	return 0;
}


int CLuaUnitScript::DropUnit(lua_State* L)
{
	// void DropUnit(int unit);
	if (activeScript == nullptr)
		return 0;

	const CUnit* transportee = ParseUnit(L, __func__, 1);

	if (transportee == nullptr)
		return 0;

	activeScript->DropUnit(transportee->id);
	return 0;
}


int CLuaUnitScript::Explode(lua_State* L)
{
	// void Explode(int piece, int flags);
	if (activeScript == nullptr)
		return 0;

	const int piece = luaL_checkint(L, 1) - 1;
	const int flags = luaL_checkint(L, 2);
	activeScript->Explode(piece, flags);
	return 0;
}


int CLuaUnitScript::ShowFlare(lua_State* L)
{
	// void ShowFlare(int piece);
	if (activeScript == nullptr)
		return 0;

	const int piece = luaL_checkint(L, 1) - 1;
	activeScript->ShowFlare(piece);
	return 0;
}


int CLuaUnitScript::Spin(lua_State* L)
{
	// void Spin(int piece, int axis, int speed, int accel);
	if (activeScript == nullptr)
		return 0;

	const int piece = luaL_checkint(L, 1) - 1;
	const int axis = ParseAxis(L, __func__, 2);
	const float speed = luaL_checkfloat(L, 3);
	const float accel = luaL_optfloat(L, 4, 0.0f); // accel == 0 -> start at desired speed immediately

	activeScript->Spin(piece, axis, speed, accel);
	return 0;
}


int CLuaUnitScript::StopSpin(lua_State* L)
{
	// void StopSpin(int piece, int axis, int decel);
	if (activeScript == nullptr)
		return 0;

	const int piece = luaL_checkint(L, 1) - 1;
	const int axis = ParseAxis(L, __func__, 2);
	const float decel = luaL_optfloat(L, 3, 0.0f); // decel == 0 -> stop immediately

	activeScript->StopSpin(piece, axis, decel);
	return 0;
}


int CLuaUnitScript::Turn(lua_State* L)
{
	// void Turn(int piece, int axis, int speed, int destination);
	// void TurnNow(int piece, int axis, int destination);
	if (activeScript == nullptr)
		return 0;

	const int piece = luaL_checkint(L, 1) - 1;
	const int axis = ParseAxis(L, __func__, 2);
	const float dest  = luaL_checkfloat(L, 3);
	const float speed = luaL_optfloat(L, 4, 0.0f); // speed == 0 -> TurnNow

	if (speed == 0.0f) {
		activeScript->TurnNow(piece, axis, dest);
	} else {
		activeScript->Turn(piece, axis, speed, dest);
	}

	return 0;
}


int CLuaUnitScript::Move(lua_State* L)
{
	// void Move(int piece, int axis, int speed, int destination);
	// void MoveNow(int piece, int axis, int destination);
	if (activeScript == nullptr)
		return 0;

	const int piece = luaL_checkint(L, 1) - 1;
	const int axis = ParseAxis(L, __func__, 2);
	const float dest  = luaL_checkfloat(L, 3);
	const float speed = luaL_optfloat(L, 4, 0.0f); // speed == 0 -> MoveNow

	if (speed == 0.0f) {
		activeScript->MoveNow(piece, axis, dest);
	} else {
		activeScript->Move(piece, axis, speed, dest);
	}

	return 0;
}


int CLuaUnitScript::IsInAnimation(lua_State* L, const char* caller, AnimType type)
{
	if (activeScript == nullptr)
		return 0;

	const int piece = luaL_checkint(L, 1) - 1;
	const int axis  = ParseAxis(L, caller, 2);

	lua_pushboolean(L, activeScript->IsInAnimation(type, piece, axis));
	return 1;
}


int CLuaUnitScript::IsInTurn(lua_State* L)
{
	return IsInAnimation(L, __func__, ATurn);
}


int CLuaUnitScript::IsInMove(lua_State* L)
{
	return IsInAnimation(L, __func__, AMove);
}


int CLuaUnitScript::IsInSpin(lua_State* L)
{
	return IsInAnimation(L, __func__, ASpin);
}


int CLuaUnitScript::WaitForAnimation(lua_State* L, const char* caller, AnimType type)
{
	if (activeScript == nullptr)
		return 0;

	CLuaUnitScript* script = dynamic_cast<CLuaUnitScript*>(activeScript);

	if (script == nullptr)
		luaL_error(L, "%s(): not a Lua unit script", caller);

	const int piece = luaL_checkint(L, 1) - 1;
	const int axis  = ParseAxis(L, caller, 2);

	lua_pushboolean(L, script->NeedsWait(type, piece, axis));
	return 1;
}


int CLuaUnitScript::WaitForTurn(lua_State* L)
{
	return WaitForAnimation(L, __func__, ATurn);
}


int CLuaUnitScript::WaitForMove(lua_State* L)
{
	return WaitForAnimation(L, __func__, AMove);
}


int CLuaUnitScript::SetDeathScriptFinished(lua_State* L)
{
	if (activeUnit == nullptr || activeScript == nullptr)
		return 0;

	CLuaUnitScript* script = dynamic_cast<CLuaUnitScript*>(activeScript);

	if (script == nullptr || !script->inKilled)
		luaL_error(L, "%s(): not a Lua unit script or 'Killed' not called", __func__);

	activeUnit->KilledScriptFinished(luaL_optint(L, 1, -1));
	return 0;
}

/******************************************************************************/

int CLuaUnitScript::GetPieceTranslation(lua_State* L)
{
	if (activeScript == nullptr)
		return 0;

	LocalModelPiece* piece = ParseLocalModelPiece(L, activeScript, __func__);
	return ToLua(L, piece->GetPosition() - piece->original->offset);
}


int CLuaUnitScript::GetPieceRotation(lua_State* L)
{
	if (activeScript == nullptr)
		return 0;

	LocalModelPiece* piece = ParseLocalModelPiece(L, activeScript, __func__);
	return ToLua(L, piece->GetRotation());
}


int CLuaUnitScript::GetPiecePosDir(lua_State* L)
{
	if (activeScript == nullptr)
		return 0;

	LocalModelPiece* piece = ParseLocalModelPiece(L, activeScript, __func__);

	float3 pos;
	float3 dir;

	if (!piece->GetEmitDirPos(pos, dir))
		return 0;

	ToLua(L, pos);
	ToLua(L, dir);
	return 6;
}

/******************************************************************************/
/******************************************************************************/

int CLuaUnitScript::GetActiveUnitID(lua_State* L)
{
	if (activeScript == nullptr)
		return 0;
	if (activeUnit == nullptr)
		return 0;

	lua_pushnumber(L, activeUnit->id);
	return 1;
}
