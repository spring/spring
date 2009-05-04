/* Author: Tobi Vollebregt */

#include "LuaUnitScript.h"

#include "LuaInclude.h"
#include "Lua/LuaHandleSynced.h"
#include "Sim/Units/UnitHandler.h"


/*

some notes:
- all piece numbers are 1 based, to be consistent with other parts of interface
- all axes are 1 based, so 1 = x, 2 = y, 3 = z
- destination, speed for Turn are in world coords, and NOT in COB coords
- destination, speed, accel, decel for Move, Spin, StopSpin are in radians
- GetUnitCOBValue(PLAY_SOUND, ...) does NOT work for Lua unit scripts,
  use Spring.PlaySound in combination with Spring.SendToUnsynced instead.


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

Spring.UnitScript.StopSpring(number unitID, number piece, number axis[, number decel]) -> nil
	Same as COB's stop-spin.  If decel isn't given spinning stops immediately.

Spring.UnitScript.Turn(number unitID, number piece, number axis, number destination[, number speed]) -> nil
	Same as COB's turn iff speed is given and not zero, and turn-now otherwise.

Spring.UnitScript.Move(number unitID, number piece, number axis, number destination[, number speed]) -> nil
	Same as COB's move iff speed is given and not zero, and move-now otherwise.



docs for callouts still to be made (prone to change):

Spring.UnitScript.IsInTurn(number unitID, number piece, number axis) -> boolean
Spring.UnitScript.IsInMove(number unitID, number piece, number axis) -> boolean
Spring.UnitScript.IsInSpin(number unitID, number piece, number axis) -> boolean
	Returns true iff such an animation exists, false otherwise.

Spring.UnitScript.CreateScript(number unitID, table callins) -> nil
	Replaces the current unit script (independent of type, also replaces COB)
	with the unit script given by a table of callins for the unit.
	Callins are similar to COB functions, e.g. a number of predefined names are
	called by the engine if they exist in the table. (Create, FireWeapon1, etc.)
	Callins do NOT take a unitID as argument, the unitID (and all script state
	should be stored in a closure.)

Spring.UnitScript.UpdateCallin(number unitID, string fname, function callin) -> nil
	Replaces or adds a single callin.  See also Spring.UnitScript.CreateScript.



docs for global LuaRules callins still to be made (prone to change):
(optionally not global but inside the callin table passed to CreateScript?)

TurnFinished(number unitID, number piece, number axis)
	Called after a turn finished for this unit/piece/axis (not a turn-now!)
	Should resume coroutine of the particular thread which called the Lua
	WaitForTurn function (see below).

MoveFinished(number unitID, number piece, number axis)
	Called after a move finished for this unit/piece/axis (not a move-now!)
	Should resume coroutine of the particular thread which called the Lua
	WaitForMove function (see below).



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
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "%s(): Bad unitID", caller);
	}
	const int unitID = lua_toint(L, index);
	if ((unitID < 0) || (static_cast<size_t>(unitID) >= uh->MaxUnits())) {
		luaL_error(L, "%s(): Bad unitID: %d\n", caller, unitID);
	}
	CUnit* unit = uh->units[unitID];
	if (unit == NULL) {
		return NULL;
	}
	return unit;
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

/******************************************************************************/
/******************************************************************************/

// moved from LuaSyncedCtrl

int CLuaUnitScript::GetUnitCOBValue(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if ((unit == NULL) || (unit->script == NULL)) {
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
	if ((unit == NULL) || (unit->script == NULL)) {
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
	if ((unit == NULL) || (unit->script == NULL)) {
		return 0;
	}

	// note: for Lua unit scripts it would be confusing if the unit's
	// unit->script->pieces differs from the unit->localmodel->pieces.

	const int piece = luaL_checkint(L, 2) - 1;
	const bool visible = lua_toboolean(L, 3);
	if ((piece < 0) || ((size_t)piece >= unit->script->pieces.size())) {
		return 0;
	}
	unit->script->SetVisibility(piece, visible);
	return 0;
}


int CLuaUnitScript::EmitSfx(lua_State* L)
{
	// void EmitSfx(int type, int piece);
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if ((unit == NULL) || (unit->script == NULL)) {
		return 0;
	}

	// note: the arguments are reversed compared to the C++ (and COB?) function

	const int piece = luaL_checkint(L, 2) - 1;
	const int type = luaL_checkint(L, 3);
	if ((piece < 0) || ((size_t)piece >= unit->script->pieces.size())) {
		return 0;
	}
	unit->script->EmitSfx(type, piece);
	return 0;
}


int CLuaUnitScript::AttachUnit(lua_State* L)
{
	// void AttachUnit(int piece, int unit);
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if ((unit == NULL) || (unit->script == NULL)) {
		return 0;
	}
	const int piece = luaL_checkint(L, 2) - 1;
	const CUnit* transportee = ParseUnit(L, __FUNCTION__, 3);
	if ((piece < 0) || ((size_t)piece >= unit->script->pieces.size()) || (transportee == NULL)) {
		return 0;
	}
	unit->script->AttachUnit(piece, transportee->id);
	return 0;
}


int CLuaUnitScript::DropUnit(lua_State* L)
{
	// void DropUnit(int unit);
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if ((unit == NULL) || (unit->script == NULL)) {
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
	if ((unit == NULL) || (unit->script == NULL)) {
		return 0;
	}
	const int piece = luaL_checkint(L, 2) - 1;
	const int flags = luaL_checkint(L, 3);
	if ((piece < 0) || ((size_t)piece >= unit->script->pieces.size())) {
		return 0;
	}
	unit->script->Explode(piece, flags);
	return 0;
}


int CLuaUnitScript::ShowFlare(lua_State* L)
{
	// void ShowFlare(int piece);
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if ((unit == NULL) || (unit->script == NULL)) {
		return 0;
	}
	const int piece = luaL_checkint(L, 2) - 1;
	if ((piece < 0) || ((size_t)piece >= unit->script->pieces.size())) {
		return 0;
	}
	unit->script->ShowFlare(piece);
	return 0;
}


int CLuaUnitScript::Spin(lua_State* L)
{
	// void Spin(int piece, int axis, int speed, int accel);
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if ((unit == NULL) || (unit->script == NULL)) {
		return 0;
	}
	const int piece = luaL_checkint(L, 2) - 1;
	if ((piece < 0) || ((size_t)piece >= unit->script->pieces.size())) {
		return 0;
	}
	const int axis  = luaL_checkint(L, 3) - 1;
	if ((axis < 0) || (axis > 2)) {
		return 0;
	}
	const float speed = luaL_checkfloat(L, 4);
	const float accel = luaL_optfloat(L, 5, 0.0f); // accel == 0 -> start at desired speed immediately

	unit->script->Spin(piece, axis, speed, accel);
	return 0;
}


int CLuaUnitScript::StopSpin(lua_State* L)
{
	// void StopSpin(int piece, int axis, int decel);
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if ((unit == NULL) || (unit->script == NULL)) {
		return 0;
	}
	const int piece = luaL_checkint(L, 2) - 1;
	if ((piece < 0) || ((size_t)piece >= unit->script->pieces.size())) {
		return 0;
	}
	const int axis  = luaL_checkint(L, 3) - 1;
	if ((axis < 0) || (axis > 2)) {
		return 0;
	}
	const float decel = luaL_optfloat(L, 4, 0.0f); // decel == 0 -> stop immediately

	unit->script->StopSpin(piece, axis, decel);
	return 0;
}


int CLuaUnitScript::Turn(lua_State* L)
{
	// void Turn(int piece, int axis, int speed, int destination);
	// void TurnNow(int piece, int axis, int destination);
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if ((unit == NULL) || (unit->script == NULL)) {
		return 0;
	}
	const int piece = luaL_checkint(L, 2) - 1;
	if ((piece < 0) || ((size_t)piece >= unit->script->pieces.size())) {
		return 0;
	}
	const int axis  = luaL_checkint(L, 3) - 1;
	if ((axis < 0) || (axis > 2)) {
		return 0;
	}
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
	if ((unit == NULL) || (unit->script == NULL)) {
		return 0;
	}
	const int piece = luaL_checkint(L, 2) - 1;
	if ((piece < 0) || ((size_t)piece >= unit->script->pieces.size())) {
		return 0;
	}
	const int axis  = luaL_checkint(L, 3) - 1;
	if ((axis < 0) || (axis > 2)) {
		return 0;
	}
	const float dest  = luaL_checkfloat(L, 4);
	const float speed = luaL_optfloat(L, 5, 0.0f); // speed == 0 -> MoveNow

	if (speed == 0.0f) {
		unit->script->MoveNow(piece, axis, dest);
	} else {
		unit->script->Move(piece, axis, speed, dest);
	}
	return 0;
}


static inline int IsInAnimation(lua_State* L, CUnitScript::AnimType wantedType)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if ((unit == NULL) || (unit->script == NULL)) {
		return 0;
	}
	const int piece = luaL_checkint(L, 2) - 1;
	if ((piece < 0) || ((size_t)piece >= unit->script->pieces.size())) {
		return 0;
	}
	const int axis  = luaL_checkint(L, 3) - 1;
	if ((axis < 0) || (axis > 2)) {
		return 0;
	}

	lua_pushboolean(L, unit->script->IsInAnimation(wantedType, piece, axis));
	return 1;
}


int CLuaUnitScript::IsInTurn(lua_State* L)
{
	return ::IsInAnimation(L, ATurn);
}


int CLuaUnitScript::IsInMove(lua_State* L)
{
	return ::IsInAnimation(L, AMove);
}


int CLuaUnitScript::IsInSpin(lua_State* L)
{
	return ::IsInAnimation(L, ASpin);
}

/******************************************************************************/
/******************************************************************************/
