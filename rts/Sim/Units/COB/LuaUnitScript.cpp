/* Author: Tobi Vollebregt */

#include "LuaUnitScript.h"

#include "LuaInclude.h"
#include "Lua/LuaHandleSynced.h"
#include "Sim/Units/UnitHandler.h"


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


int CLuaUnitScript::PieceEmitSfx(lua_State* L)
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


int CLuaUnitScript::PieceAttachUnit(lua_State* L)
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


int CLuaUnitScript::PieceDropUnit(lua_State* L)
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


int CLuaUnitScript::PieceExplode(lua_State* L)
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


int CLuaUnitScript::PieceShowFlare(lua_State* L)
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


int CLuaUnitScript::PieceSpin(lua_State* L)
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


int CLuaUnitScript::PieceStopSpin(lua_State* L)
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


int CLuaUnitScript::PieceTurn(lua_State* L)
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


int CLuaUnitScript::PieceMove(lua_State* L)
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

/******************************************************************************/
/******************************************************************************/
