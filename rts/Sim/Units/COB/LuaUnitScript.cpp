/* Author: Tobi Vollebregt */

#define DEBUG_LUA 0

#include "LuaUnitScript.h"

#include "CobInstance.h"
#include "LogOutput.h"
#include "LuaInclude.h"
#include "NullUnitScript.h"
#include "UnitScriptNames.h"
#include "Lua/LuaCallInCheck.h"
#include "Lua/LuaHandleSynced.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Weapons/PlasmaRepulser.h"


using std::map;
using std::pair;
using std::string;
using std::vector;


/*

some notes:
- all piece numbers are 1 based, to be consistent with other parts of interface
- all axes are 1 based, so 1 = x, 2 = y, 3 = z
- destination, speed for Move are in world coords, and NOT in COB coords
- therefore, compared to COB, the X axis for the Move callout is mirrored
- destination, speed, accel, decel for Turn, Spin, StopSpin are in radians
- GetUnitCOBValue(PLAY_SOUND, ...) does NOT work for Lua unit scripts,
  use Spring.PlaySound in combination with Spring.SendToUnsynced instead.
- Because in current design CBCobThreadFinish can impossibly be called, certain
  state changes which normally happen immediately when script returns should
  be triggered through a call to a callOut when using Lua scripts.
  This applies to:
  * Spring.SetUnitShieldState(unitID, false|true) replaces return value 0|1 of
    COB's AimWeaponX function for plasma repulsers.
  * Spring.SetUnitWeaponState(unitID, weaponNum, "aimReady", 0|1) replaces
    return value 0|1 of COB's AimWeaponX function for all other weapons.
  * Spring.UnitScript.SetDeathScriptFinished(unitID, wreckLevel) replaces
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

TurnFinished(number unitID, number piece, number axis)
	Called after a turn finished for this unit/piece/axis (not a turn-now!)
	Should resume coroutine of the particular thread which called the Lua
	WaitForTurn function (see below).

MoveFinished(number unitID, number piece, number axis)
	Called after a move finished for this unit/piece/axis (not a move-now!)
	Should resume coroutine of the particular thread which called the Lua
	WaitForMove function (see below).


docs for callouts defined in this file:

Spring.UnitScript.SetUnitCOBValue(...)
	see wiki for Spring.GetUnitCOBValue (unchanged)

Spring.UnitScript.GetUnitCOBValue(...)
	see wiki for Spring.GetUnitCOBValue (unchanged)

Spring.UnitScript.SetPieceVisibility(number unitID, number piece, boolean visible) -> nil
	Set's piece visibility.  Same as COB's hide/show.

Spring.UnitSript.EmitSfx(number unitID, number piece, number type) -> nil
	Same as COB's emit-sfx.

Spring.UnitScript.AttachUnit(number unitID, number piece, number transporteeID) -> nil
	Same as COB's attach-unit.

Spring.UnitScript.DropUnit(number unitID, number transporteeID) -> nil
	Same as COB's drop-unit.

Spring.UnitScript.Explode(number unitID, number piece, number flags) -> nil
	Same as COB's explode.

Spring.UnitScript.ShowFlare(number unitID, number piece) -> nil
	Same as COB's show _inside_ FireWeaponX.

Spring.UnitScript.Spin(number unitID, number piece, number axis, number speed[, number accel]) -> nil
	Same as COB's spin.  If accel isn't given spinning starts at the desired speed.

Spring.UnitScript.StopSpinning(number unitID, number piece, number axis[, number decel]) -> nil
	Same as COB's stop-spin.  If decel isn't given spinning stops immediately.

Spring.UnitScript.Turn(number unitID, number piece, number axis, number destination[, number speed]) -> nil
	Same as COB's turn iff speed is given and not zero, and turn-now otherwise.

Spring.UnitScript.Move(number unitID, number piece, number axis, number destination[, number speed]) -> nil
	Same as COB's move iff speed is given and not zero, and move-now otherwise.

Spring.UnitScript.IsInTurn(number unitID, number piece, number axis) -> boolean
Spring.UnitScript.IsInMove(number unitID, number piece, number axis) -> boolean
Spring.UnitScript.IsInSpin(number unitID, number piece, number axis) -> boolean
	Returns true iff such an animation exists, false otherwise.

Spring.UnitScript.WaitForTurn(number unitID, number piece, number axis) -> boolean
	Returns true iff such an animation exists, false otherwise.  Iff it returns
	true, the TurnFinished callIn will be called once the turn completes.

Spring.UnitScript.WaitForMove(number unitID, number piece, number axis) -> boolean
	Returns true iff such an animation exists, false otherwise.  Iff it returns
	true, the MoveFinished callIn will be called once the move completes.

Spring.UnitScript.SetDeathScriptFinished(number unitID[, number wreckLevel])
	Tells Spring the Killed script finished, and which wreckLevel to use.
	If wreckLevel is not given no wreck is created.

Spring.UnitScript.CreateScript(number unitID, table callIns) -> nil
	Replaces the current unit script (independent of type, also replaces COB)
	with the unit script given by a table of callins for the unit.
	Callins are similar to COB functions, e.g. a number of predefined names are
	called by the engine if they exist in the table. (Create, FireWeapon1, etc.)

Spring.UnitScript.UpdateCallIn(number unitID, string fname[, function callIn]) -> number|boolean
	Iff callIn is a function, a single callIn is replaced or added, and the
	new functionID is returned.  If callIn isn't given or is nil, the callIn is
	nilled, returns true if it was removed, or false if the callin didn't exist.
	See also Spring.UnitScript.CreateScript.


random prototype snippets of Lua framework code:

function WaitForTurn(unitID, piece, axis)
	--the code which resumed the coroutine is responsible for processing
	--this result and putting the "thread" in the 'waitForTurn' queue.
	coroutine.yield(unitID, "turn", piece, axis)
end

function WaitForMove(unitID, piece, axis)
	--idem
	coroutine.yield(unitID, "move", piece, axis)
end

function Sleep(unitID, delay)
	--needs check for sleep smaller then a single frame
	coroutine.yield(unitID, "sleep", math.floor(delay / 33))
end

function gagdet:GameFrame(f)
	if (sleepQue[f] ~= nil) then
		--resume all coroutines scheduled to run in this frame and process results
	end
end

*/


#if DEBUG_LUA
#  define LUA_TRACE(m) do { if (unit) logOutput.Print("%s: %d: %s", __FUNCTION__, unit->id, m); \
                            else logOutput.Print("%s: %s", __FUNCTION__, m); \
                       } while (false);
#else
#  define LUA_TRACE(m)
#endif


// TODO: move this (LuaUtils?)
// debugging, pretty much copied from Lua book
static void StackDump(lua_State* L)
{
	int top = lua_gettop(L);
	for (int i = 1; i <= top; ++i) {
		int t = lua_type(L, i);
		switch (t) {
			case LUA_TSTRING:
				logOutput.Print("%2d/%3d: '%s'", i, i - top - 1, lua_tostring(L, i));
				break;
			case LUA_TBOOLEAN:
				logOutput.Print("%2d/%3d: %s", i, i - top - 1, lua_toboolean(L, i) ? "true" : "false");
				break;
			case LUA_TNUMBER:
				logOutput.Print("%2d/%3d: %g", i, i - top - 1, lua_tonumber(L, i));
				break;
			default:
				logOutput.Print("%2d/%3d: %s", i, i - top - 1, lua_typename(L, t));
				break;
		}
	}
}


/******************************************************************************/
/******************************************************************************/


CLuaUnitScript::CLuaUnitScript(lua_State* L, CUnit* unit)
	: CUnitScript(unit, scriptIndex, unit->localmodel->pieces)
	, handle(CLuaHandle::activeHandle), L(L)
	, scriptIndex(COBFN_Last + (unit->weapons.size() * COBFN_Weapon_Funcs), LUA_NOREF)
	, inKilled(false)
{
	for (lua_pushnil(L); lua_next(L, 2) != 0; /*lua_pop(L, 1)*/) {
		const string fname = lua_tostring(L, -2);
		const int r = luaL_ref(L, LUA_REGISTRYINDEX);

		scriptNames.insert(pair<string, int>(fname, r));
		UpdateCallIn(fname, r);
	}
}


CLuaUnitScript::~CLuaUnitScript()
{
	// if L is NULL then the lua_State is closed/closing (see HandleFreed)
	if (L != NULL) {
		for (map<string, int>::iterator it = scriptNames.begin(); it != scriptNames.end(); ++it) {
			luaL_unref(L, LUA_REGISTRYINDEX, it->second);
		}
	}
}


void CLuaUnitScript::HandleFreed(CLuaHandle* handle)
{
	std::list<CUnit*>::iterator ui;
	for (ui = uh->activeUnits.begin(); ui != uh->activeUnits.end(); ++ui) {
		CLuaUnitScript* script = dynamic_cast<CLuaUnitScript*>((*ui)->script);

		// kill only the Lua scripts running in this handle
		if (script != NULL && script->handle == handle) {

			// we don't have anything better ...
			(*ui)->script = new CNullUnitScript(*ui);

			// signal the destructor it shouldn't unref refs
			script->L = NULL;

			delete script;
		}
	}
}


int CLuaUnitScript::UpdateCallIn()
{
	const string fname = lua_tostring(L, 2);
	const bool remove = lua_isnoneornil(L, 3);
	map<string, int>::iterator it = scriptNames.find(fname);
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
		scriptNames.insert(pair<string, int>(fname, r));
	}

	UpdateCallIn(fname, r);

	if (!remove) {
		// the reference doubles as the functionId, as expected by RealCall
		// from Lua this can be used with e.g. Spring.CallCOBScript
		lua_pushnumber(L, r);
	}
	return 1;
}


void CLuaUnitScript::UpdateCallIn(const string& fname, int ref)
{
	// Map common function names to indices
	int num = CUnitScriptNames::GetScriptNumber(fname);

	// Check upper bound too in case user calls UpdateCallIn with nonexisting weapon.
	// (we only allocate slots in scriptIndex for the number of weapons the unit has)
	if (num >= 0 && num < int(scriptIndex.size())) {
		scriptIndex[num] = ref;
	}

	LUA_TRACE(fname.c_str());
}


void CLuaUnitScript::RemoveCallIn(const string& fname)
{
	map<string, int>::iterator it = scriptNames.find(fname);

	if (it != scriptNames.end()) {
		luaL_unref(L, LUA_REGISTRYINDEX, it->second);
		scriptNames.erase(it);
		UpdateCallIn(fname, LUA_NOREF);
	}

	LUA_TRACE(fname.c_str());
}


void CLuaUnitScript::ShowScriptError(const string& msg)
{
	// if we are in the same handle, we can truely raise an error
	if (CLuaHandle::activeHandle == handle) {
		luaL_error(L, "Lua UnitScript error: %s", msg.c_str());
	}
	else {
		logOutput.Print("Lua UnitScript error: %s", msg.c_str());
	}
}


/******************************************************************************/
/******************************************************************************/


inline float CLuaUnitScript::PopNumber(int fn, float def)
{
	if (!lua_israwnumber(L, -1)) {
		const string& fname = CUnitScriptNames::GetScriptName(fn);

		logOutput.Print("%s: bad return value, expected number", fname.c_str());
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
	if (!lua_israwnumber(L, -1)) {
		const string& fname = CUnitScriptNames::GetScriptName(fn);

		logOutput.Print("%s: bad return value, expected boolean", fname.c_str());
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

	// Push unitID on the stack (all callIns get this)
	lua_pushnumber(L, unit->id);
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
	if (!HasFunction(fn)) {
		return -1;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);

	PushFunction(fn);

	if (!RunCallIn(fn, 1, 1)) {
		return -1;
	}

	return int(PopNumber(fn, -1));
}


void CLuaUnitScript::Call(int fn, float arg1)
{
	if (!HasFunction(fn)) {
		return;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);

	PushFunction(fn);
	lua_pushnumber(L, arg1);

	RunCallIn(fn, 2, 0);
}


void CLuaUnitScript::Call(int fn, float arg1, float arg2)
{
	if (!HasFunction(fn)) {
		return;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);

	PushFunction(fn);
	lua_pushnumber(L, arg1);
	lua_pushnumber(L, arg2);

	RunCallIn(fn, 3, 0);
}


/******************************************************************************/
/******************************************************************************/


void CLuaUnitScript::Create()
{
	Call(COBFN_Create);
}


void CLuaUnitScript::Killed()
{
	const int fn = COBFN_Killed;

	if (!HasFunction(fn)) {
		unit->deathScriptFinished = true;
		//FIXME: unit->delayedWreckLevel = ???
		return;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);

	PushFunction(fn);
	lua_pushnumber(L, unit->recentDamage);
	lua_pushnumber(L, unit->maxHealth);

	inKilled = true;

	if (!RunCallIn(fn, 3, 1)) {
		return;
	}

	// If Killed returns an integer, it signals it hasn't started a thread.
	// In this case the return value is the delayedWreckLevel.
	if (lua_israwnumber(L, -1)) {
		inKilled = false;
		unit->deathScriptFinished = true;
		unit->delayedWreckLevel = lua_toint(L, -1);
	}
	else if (!lua_isnoneornil(L, -1)) {
		const string& fname = CUnitScriptNames::GetScriptName(fn);

		logOutput.Print("%s: bad return value, expected number or nil", fname.c_str());
		RemoveCallIn(fname);

		// without this we would end up with zombie units
		unit->deathScriptFinished = true;
		//FIXME: unit->delayedWreckLevel = ???
	}

	lua_pop(L, 1);
}


void CLuaUnitScript::SetDirection(float heading)
{
	Call(COBFN_SetDirection, heading);
}


void CLuaUnitScript::SetSpeed(float speed, float)
{
	Call(COBFN_SetSpeed, speed);
}


void CLuaUnitScript::RockUnit(const float3& rockDir)
{
	//FIXME: maybe we want rockDir.y too to be future proof?
	Call(COBFN_RockUnit, rockDir.x, rockDir.z);
}


void CLuaUnitScript::HitByWeapon(const float3& hitDir)
{
	//FIXME: maybe we want hitDir.y too to be future proof?
	Call(COBFN_HitByWeapon, hitDir.x, hitDir.z);
}


void CLuaUnitScript::HitByWeaponId(const float3& hitDir, int weaponDefId, float& inout_damage)
{
	const int fn = COBFN_HitByWeaponId;

	if (!HasFunction(fn)) {
		return;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 6);

	PushFunction(fn);
	lua_pushnumber(L, hitDir.x);
	//FIXME: maybe we want hitDir.y too to be future proof?
	lua_pushnumber(L, hitDir.z);
	lua_pushnumber(L, weaponDefId);
	lua_pushnumber(L, inout_damage);

	if (!RunCallIn(fn, 5, 1)) {
		return;
	}

	if (lua_israwnumber(L, -1)) {
		inout_damage = lua_tonumber(L, -1);
	}
	else if (!lua_isnoneornil(L, -1)) {
		const string& fname = CUnitScriptNames::GetScriptName(fn);

		logOutput.Print("%s: bad return value, expected number or nil", fname.c_str());
		RemoveCallIn(fname);
	}

	lua_pop(L, 1);
}


void CLuaUnitScript::SetSFXOccupy(int curTerrainType)
{
	const int fn = COBFN_SetSFXOccupy;

	if (!HasFunction(fn)) {
		return;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);

	PushFunction(fn);
	lua_pushnumber(L, curTerrainType);

	RunCallIn(fn, 2, 0);
}


void CLuaUnitScript::QueryLandingPads(std::vector<int>& out_pieces)
{
	const int fn = COBFN_QueryLandingPad;

	if (!HasFunction(fn)) {
		return;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);

	PushFunction(fn);

	if (!RunCallIn(fn, 1, 1)) {
		return;
	}

	if (lua_istable(L, -1)) {
		int n = 1;
		lua_rawgeti(L, -1, n);
		while (lua_israwnumber(L, -1)) {
			out_pieces.push_back(lua_toint(L, -1));
			lua_pop(L, 1);
			lua_rawgeti(L, -1, ++n);
		}
		lua_pop(L, 1);
	}
	else {
		const string& fname = CUnitScriptNames::GetScriptName(fn);

		logOutput.Print("%s: bad return value, expected table", fname.c_str());
		RemoveCallIn(fname);
	}

	lua_pop(L, 1);
}


void CLuaUnitScript::BeginTransport(const CUnit* unit)
{
	Call(COBFN_BeginTransport, unit->id);
}


int CLuaUnitScript::QueryTransport(const CUnit* unit)
{
	const int fn = COBFN_QueryTransport;

	if (!HasFunction(fn)) {
		return -1;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);

	PushFunction(fn);
	lua_pushnumber(L, unit->id);

	if (!RunCallIn(fn, 2, 1)) {
		return -1;
	}

	return int(PopNumber(fn, -1));
}


void CLuaUnitScript::TransportPickup(const CUnit* unit)
{
	Call(COBFN_TransportPickup, unit->id);
}


void CLuaUnitScript::TransportDrop(const CUnit* unit, const float3& pos)
{
	const int fn = COBFN_TransportDrop;

	if (!HasFunction(fn)) {
		return;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 6);

	PushFunction(fn);
	lua_pushnumber(L, unit->id);
	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);

	RunCallIn(fn, 5, 0);
}


void CLuaUnitScript::StartBuilding(float heading, float pitch)
{
	Call(COBFN_StartBuilding, heading, pitch);
}


int CLuaUnitScript::QueryNanoPiece()
{
	return RunQueryCallIn(COBFN_QueryNanoPiece);
}


int CLuaUnitScript::QueryBuildInfo()
{
	return RunQueryCallIn(COBFN_QueryBuildInfo);
}


int CLuaUnitScript::QueryWeapon(int weaponNum)
{
	return RunQueryCallIn(COBFN_QueryPrimary + COBFN_Weapon_Funcs * weaponNum);
}


void CLuaUnitScript::AimWeapon(int weaponNum, float heading, float pitch)
{
	Call(COBFN_AimPrimary + COBFN_Weapon_Funcs * weaponNum, heading, pitch);
}


void  CLuaUnitScript::AimShieldWeapon(CPlasmaRepulser* weapon)
{
	Call(COBFN_AimPrimary + COBFN_Weapon_Funcs * weapon->weaponNum);
}


int CLuaUnitScript::AimFromWeapon(int weaponNum)
{
	return RunQueryCallIn(COBFN_AimFromPrimary + COBFN_Weapon_Funcs * weaponNum);
}


void CLuaUnitScript::Shot(int weaponNum)
{
	// FIXME: pass projectileID?
	Call(COBFN_Shot + COBFN_Weapon_Funcs * weaponNum);
}


bool CLuaUnitScript::BlockShot(int weaponNum, const CUnit* targetUnit, bool userTarget)
{
	const int fn = COBFN_BlockShot + COBFN_Weapon_Funcs * weaponNum;

	if (!HasFunction(fn)) {
		return false;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);

	PushFunction(fn);
	PushUnit(targetUnit);
	lua_pushboolean(L, userTarget);

	if (!RunCallIn(fn, 3, 1)) {
		return false;
	}

	return PopBoolean(fn, false);
}


float CLuaUnitScript::TargetWeight(int weaponNum, const CUnit* targetUnit)
{
	const int fn = COBFN_TargetWeight + COBFN_Weapon_Funcs * weaponNum;

	if (!HasFunction(fn)) {
		return 1.0f;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);

	PushFunction(fn);
	PushUnit(targetUnit);

	if (!RunCallIn(fn, 2, 1)) {
		return 1.0f;
	}

	return PopNumber(fn, 1.0f);
}


void CLuaUnitScript::AnimFinished(AnimType type, int piece, int axis)
{
	const int fn = (type == AMove ? COBFN_MoveFinished : COBFN_TurnFinished);

	Call(fn, piece + 1, axis + 1);
}


void CLuaUnitScript::RawCall(int functionId)
{
	if (functionId < 0) {
		return;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);

	RawPushFunction(functionId);
	RawRunCallIn(functionId, 1, 0);
}


string CLuaUnitScript::GetScriptName(int functionId) const
{
	// only for error messages, so speed doesn't matter
	map<string, int>::const_iterator it = scriptNames.begin();
	for (; it != scriptNames.end(); ++it) {
		if (it->second == functionId) return it->first;
	}
	std::stringstream s;
	s << "<unnamed: " << functionId << ">";
	return s.str();
}


bool CLuaUnitScript::RawRunCallIn(int functionId, int inArgs, int outArgs)
{
	std::string err;
	const int error = handle->RunCallIn(inArgs, outArgs, err);

	if (error != 0) {
		const string& fname = GetScriptName(functionId);

		logOutput.Print("%s::RunCallIn: error = %i, %s::%s, %s\n",
		                handle->GetName().c_str(), error, "CLuaUnitScript",
		                fname.c_str(), err.c_str());
		RemoveCallIn(fname);

		return false;
	}
	return true;
}


/******************************************************************************/
/******************************************************************************/


static void PushEntry(lua_State* L, const char* name, lua_CFunction fun)
{
	lua_pushstring(L, name);
	lua_pushcfunction(L, fun);
	//lua_pushvalue(L, -5);      // push the environment table
	//if (lua_setfenv(L, -2) != 1) { assert(false); }
	lua_rawset(L, -3);
}


bool CLuaUnitScript::PushEntries(lua_State* L)
{
	//lua_newtable(L);   // environment table for all C functions in this file
	lua_pushstring(L, "UnitScript");
	lua_newtable(L);

#define REGISTER_LUA_CFUNC(x) \
	PushEntry(L, #x, x)

	REGISTER_LUA_CFUNC(CreateScript);
	REGISTER_LUA_CFUNC(UpdateCallIn);

	REGISTER_LUA_CFUNC(GetUnitCOBValue);
	REGISTER_LUA_CFUNC(SetUnitCOBValue);
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

	lua_rawset(L, -3);
	//lua_pop(L, 1);     // pop the environment table
	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Access helpers
//

// FIXME: this badly needs a clean up, it's duplicated

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


/******************************************************************************/
/******************************************************************************/
//
//  Parsing helpers
//

// FIXME: this badly needs a clean up, it's duplicated

static inline CUnit* ParseRawUnit(lua_State* L, const char* caller, int index)
{
	if (!lua_israwnumber(L, index)) {
		luaL_error(L, "%s(): Bad unitID", caller);
	}
	const int unitID = lua_toint(L, index);
	if ((unitID < 0) || (static_cast<size_t>(unitID) >= uh->MaxUnits())) {
		luaL_error(L, "%s(): Bad unitID: %d\n", caller, unitID);
	}
	return uh->units[unitID];
}


static inline CUnit* ParseUnit(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseRawUnit(L, caller, index);
	if (unit == NULL) {
		return NULL;
	}
	if (!CanControlUnit(unit)) {
		return NULL;
	}
	return unit;
}


static inline int ParseAxis(lua_State* L, const char* caller, int index)
{
	if (!lua_israwnumber(L, index)) {
		luaL_error(L, "%s(): Bad axis", caller);
	}
	const int axis  = lua_toint(L, index) - 1;
	if ((axis < 0) || (axis > 2)) {
		luaL_error(L, "%s(): Bad axis", caller);
	}
	return axis;
}


/******************************************************************************/
/******************************************************************************/


int CLuaUnitScript::CreateScript(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		LUA_TRACE("no such unit");
		return 0;
	}

	// check table of callIns
	// (we might not get a chance to clean up later on, if something is wrong)
	if (!lua_istable(L, 2)) {
		luaL_error(L, "%s(): error parsing callIn table", __FUNCTION__);
	}
	for (lua_pushnil(L); lua_next(L, 2) != 0; lua_pop(L, 1)) {
		if (!lua_israwstring(L, -2) || !lua_isfunction(L, -1)) {
			luaL_error(L, "%s(): error parsing callIn table", __FUNCTION__);
		}
	}

	// replace the unit's script (ctor parses callIn table)
	delete unit->script;
	unit->script = new CLuaUnitScript(L, unit);

	LUA_TRACE("script replaced with CLuaUnitScript");

	// Since we can only be created from Lua,
	// Create would never be called otherwise
	unit->script->Create();

	return 0;
}


int CLuaUnitScript::UpdateCallIn(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	CLuaUnitScript* script = dynamic_cast<CLuaUnitScript*>(unit->script);
	if (script == NULL) {
		luaL_error(L, "%s(): not a Lua unit script", __FUNCTION__);
	}
	// we would get confused if our refs aren't together in a single state
	if (L != script->L) {
		luaL_error(L, "%s(): incorrect lua_State", __FUNCTION__);
	}
	if (!lua_israwstring(L, 2) || (!lua_isfunction(L, 3) && !lua_isnoneornil(L, 3))) {
		luaL_error(L, "Incorrect arguments to %s()", __FUNCTION__);
	}

	return script->UpdateCallIn();
}


// moved from LuaSyncedCtrl

int CLuaUnitScript::GetUnitCOBValue(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	int arg = 2;
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

	const int result = unit->script->GetUnitVal(val, p[0], p[1], p[2], p[3]);
	if (!splitData) {
		lua_pushnumber(L, result);
		return 1;
	}
	lua_pushnumber(L, UNPACKX(result));
	lua_pushnumber(L, UNPACKZ(result));
	return 2;
}


// moved from LuaSyncedCtrl

int CLuaUnitScript::SetUnitCOBValue(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	const int val = luaL_checkint(L, 2);
	int param;
	if (args == 3) {
		param = luaL_checkint(L, 3);
	}
	else {
		const int x = luaL_checkint(L, 3);
		const int z = luaL_checkint(L, 4);
		param = PACKXZ(x, z);
	}
	unit->script->SetUnitVal(val, param);
	return 0;
}


int CLuaUnitScript::SetPieceVisibility(lua_State* L)
{
	// void SetVisibility(int piece, bool visible);
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	// note: for Lua unit scripts it would be confusing if the unit's
	// unit->script->pieces differs from the unit->localmodel->pieces.

	const int piece = luaL_checkint(L, 2) - 1;
	const bool visible = lua_toboolean(L, 3);
	unit->script->SetVisibility(piece, visible);
	return 0;
}


int CLuaUnitScript::EmitSfx(lua_State* L)
{
	// void EmitSfx(int type, int piece);
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	// note: the arguments are reversed compared to the C++ (and COB?) function

	const int piece = luaL_checkint(L, 2) - 1;
	const int type = luaL_checkint(L, 3);
	unit->script->EmitSfx(type, piece);
	return 0;
}


int CLuaUnitScript::AttachUnit(lua_State* L)
{
	// void AttachUnit(int piece, int unit);
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int piece = luaL_checkint(L, 2) - 1;
	const CUnit* transportee = ParseUnit(L, __FUNCTION__, 3);
	if (transportee == NULL) {
		return 0;
	}
	unit->script->AttachUnit(piece, transportee->id);
	return 0;
}


int CLuaUnitScript::DropUnit(lua_State* L)
{
	// void DropUnit(int unit);
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const CUnit* transportee = ParseUnit(L, __FUNCTION__, 2);
	if (transportee == NULL) {
		return 0;
	}
	unit->script->DropUnit(transportee->id);
	return 0;
}


int CLuaUnitScript::Explode(lua_State* L)
{
	// void Explode(int piece, int flags);
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int piece = luaL_checkint(L, 2) - 1;
	const int flags = luaL_checkint(L, 3);
	unit->script->Explode(piece, flags);
	return 0;
}


int CLuaUnitScript::ShowFlare(lua_State* L)
{
	// void ShowFlare(int piece);
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int piece = luaL_checkint(L, 2) - 1;
	unit->script->ShowFlare(piece);
	return 0;
}


int CLuaUnitScript::Spin(lua_State* L)
{
	// void Spin(int piece, int axis, int speed, int accel);
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int piece = luaL_checkint(L, 2) - 1;
	const int axis = ParseAxis(L, __FUNCTION__, 3);
	const float speed = luaL_checkfloat(L, 4);
	const float accel = luaL_optfloat(L, 5, 0.0f); // accel == 0 -> start at desired speed immediately

	unit->script->Spin(piece, axis, speed, accel);
	return 0;
}


int CLuaUnitScript::StopSpin(lua_State* L)
{
	// void StopSpin(int piece, int axis, int decel);
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int piece = luaL_checkint(L, 2) - 1;
	const int axis = ParseAxis(L, __FUNCTION__, 3);
	const float decel = luaL_optfloat(L, 4, 0.0f); // decel == 0 -> stop immediately

	unit->script->StopSpin(piece, axis, decel);
	return 0;
}


int CLuaUnitScript::Turn(lua_State* L)
{
	// void Turn(int piece, int axis, int speed, int destination);
	// void TurnNow(int piece, int axis, int destination);
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int piece = luaL_checkint(L, 2) - 1;
	const int axis = ParseAxis(L, __FUNCTION__, 3);
	const float dest  = luaL_checkfloat(L, 4);
	const float speed = luaL_optfloat(L, 5, 0.0f); // speed == 0 -> TurnNow

	if (speed == 0.0f) {
		unit->script->TurnNow(piece, axis, dest);
	} else {
		unit->script->Turn(piece, axis, speed, dest);
	}
	return 0;
}


int CLuaUnitScript::Move(lua_State* L)
{
	// void Move(int piece, int axis, int speed, int destination);
	// void MoveNow(int piece, int axis, int destination);
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int piece = luaL_checkint(L, 2) - 1;
	const int axis = ParseAxis(L, __FUNCTION__, 3);
	const float dest  = luaL_checkfloat(L, 4);
	const float speed = luaL_optfloat(L, 5, 0.0f); // speed == 0 -> MoveNow

	if (speed == 0.0f) {
		unit->script->MoveNow(piece, axis, dest);
	} else {
		unit->script->Move(piece, axis, speed, dest);
	}
	return 0;
}


int CLuaUnitScript::IsInAnimation(lua_State* L, const char* caller, AnimType type)
{
	CUnit* unit = ParseUnit(L, caller, 1);
	if (unit == NULL) {
		return 0;
	}
	const int piece = luaL_checkint(L, 2) - 1;
	const int axis  = ParseAxis(L, caller, 3);

	lua_pushboolean(L, unit->script->IsInAnimation(type, piece, axis));
	return 1;
}


int CLuaUnitScript::IsInTurn(lua_State* L)
{
	return IsInAnimation(L, __FUNCTION__, ATurn);
}


int CLuaUnitScript::IsInMove(lua_State* L)
{
	return IsInAnimation(L, __FUNCTION__, AMove);
}


int CLuaUnitScript::IsInSpin(lua_State* L)
{
	return IsInAnimation(L, __FUNCTION__, ASpin);
}


int CLuaUnitScript::WaitForAnimation(lua_State* L, const char* caller, AnimType type)
{
	CUnit* unit = ParseUnit(L, caller, 1);
	if (unit == NULL) {
		return 0;
	}
	CLuaUnitScript* script = dynamic_cast<CLuaUnitScript*>(unit->script);
	if (script == NULL) {
		luaL_error(L, "%s(): not a Lua unit script", caller);
	}
	const int piece = luaL_checkint(L, 2) - 1;
	const int axis  = ParseAxis(L, caller, 3);

	lua_pushboolean(L, script->AddAnimListener(type, piece, axis, script));
	return 1;
}


int CLuaUnitScript::WaitForTurn(lua_State* L)
{
	return WaitForAnimation(L, __FUNCTION__, ATurn);
}


int CLuaUnitScript::WaitForMove(lua_State* L)
{
	return WaitForAnimation(L, __FUNCTION__, AMove);
}


int CLuaUnitScript::SetDeathScriptFinished(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	CLuaUnitScript* script = dynamic_cast<CLuaUnitScript*>(unit->script);
	if (script == NULL || !script->inKilled) {
		luaL_error(L, "%s(): not a Lua unit script or 'Killed' not called", __FUNCTION__);
	}
	unit->deathScriptFinished = true;
	unit->delayedWreckLevel = luaL_optint(L, 2, -1);
	return 0;
}


/******************************************************************************/
/******************************************************************************/
