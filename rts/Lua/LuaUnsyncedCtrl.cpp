#include "StdAfx.h"
// LuaUnsyncedCtrl.cpp: implementation of the LuaUnsyncedCtrl class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaUnsyncedCtrl.h"
#include <set>
#include <list>
#include <cctype>
using namespace std;

#include "LuaInclude.h"

#include "LuaHandle.h"
#include "LuaHashString.h"

#include "Game/Camera.h"
#include "Game/CameraController.h"
#include "Game/SelectedUnits.h"
#include "Game/UI/CursorIcons.h"
#include "Game/UI/MouseHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "System/LogOutput.h"
#include "System/Sound.h"


/******************************************************************************/
/******************************************************************************/

bool LuaUnsyncedCtrl::PushEntries(lua_State* L)
{
#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(Echo);

	REGISTER_LUA_CFUNC(PlaySoundFile);

	REGISTER_LUA_CFUNC(SetCameraState);
	REGISTER_LUA_CFUNC(SetCameraTarget);

	REGISTER_LUA_CFUNC(SelectUnitMap);
	REGISTER_LUA_CFUNC(SelectUnitArray);

	REGISTER_LUA_CFUNC(AddWorldIcon);
	REGISTER_LUA_CFUNC(AddWorldText);
	REGISTER_LUA_CFUNC(AddWorldUnit);

	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Access helpers
//

static inline bool FullCtrl()
{
	return CLuaHandle::GetActiveHandle()->GetFullCtrl();
}


static inline int CtrlTeam()
{
	return CLuaHandle::GetActiveHandle()->GetCtrlTeam();
}


static inline int CtrlAllyTeam()
{
	const int ctrlTeam = CtrlTeam();
	if (ctrlTeam < 0) {
		return ctrlTeam;
	}
	return gs->AllyTeam(ctrlTeam);
}


static inline bool CanCtrlTeam(int team)
{
	const int ctrlTeam = CtrlTeam();
	if (ctrlTeam < 0) {
		return (ctrlTeam == CLuaHandle::AllAccessTeam) ? true : false;
	}
	return (ctrlTeam == team);
}


static inline bool CanCtrlAllyTeam(int allyteam)
{
	const int ctrlTeam = CtrlTeam();
	if (ctrlTeam < 0) {
		return (ctrlTeam == CLuaHandle::AllAccessTeam) ? true : false;
	}
	return (gs->AllyTeam(ctrlTeam) == allyteam);
}


/******************************************************************************/
/******************************************************************************/
//
//  Parsing helpers
//

static inline CUnit* ParseSelectUnit(lua_State* L,
                                     const char* caller, int luaIndex)
{
	if (!lua_isnumber(L, luaIndex)) {
		luaL_error(L, "%s(): Bad unitID", caller);
	}
	const int unitID = (int)lua_tonumber(L, luaIndex);
	if ((unitID < 0) || (unitID >= MAX_UNITS)) {
		luaL_error(L, "%s(): Bad unitID: %i\n", caller, unitID);
	}
	CUnit* unit = uh->units[unitID];
	if (unit == NULL) {
		return NULL;
	}
	const int selectTeam = CLuaHandle::GetActiveHandle()->GetSelectTeam();
	if (selectTeam < 0) {
		return (selectTeam == CLuaHandle::AllAccessTeam) ? unit : NULL;
	}
	if (selectTeam == unit->team) {
		return unit;
	}
	return NULL;
}


/******************************************************************************/
/******************************************************************************/
//
//  The call-outs
//

int LuaUnsyncedCtrl::Echo(lua_State* L)
{
	// copied from lua/src/lib/lbaselib.c
	string msg = "";
	const int args = lua_gettop(L); // number of arguments

	lua_getglobal(L, "tostring");

	for (int i = 1; i <= args; i++) {
		const char *s;
		lua_pushvalue(L, -1);     // function to be called
		lua_pushvalue(L, i);      // value to print
		lua_call(L, 1, 1);
		s = lua_tostring(L, -1);  // get result
		if (s == NULL) {
			return luaL_error(L, "`tostring' must return a string to `print'");
		}
		if (i > 1) {
			msg += ", ";
		}
		msg += s;
		lua_pop(L, 1);            // pop result
	}
	logOutput.Print(msg);

	if ((args != 1) || !lua_istable(L, 1)) {
		return 0;
	}

	// print solo tables (array style)
	msg = "TABLE: ";
	bool first = true;
	const int table = 1;
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_isnumber(L, -2)) {  // only numeric keys
			const char *s;
			lua_pushvalue(L, -3);     // function to be called
			lua_pushvalue(L, -2	);    // value to print
			lua_call(L, 1, 1);
			s = lua_tostring(L, -1);  // get result
			if (s == NULL) {
				return luaL_error(L, "`tostring' must return a string to `print'");
			}
			if (!first) {
				msg += ", ";
			}
			msg += s;
			first = false;
			lua_pop(L, 1);            // pop result
		}
	}
	logOutput.Print(msg);

	return 0;
}


int LuaUnsyncedCtrl::PlaySoundFile(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to PlaySoundFile()");
	}
	const string soundFile = lua_tostring(L, 1);
	const unsigned int soundID = sound->GetWaveId(soundFile, false);
	if (soundID > 0) {
		float volume = 1.0f;
		if (args >= 2) {
			volume = (float)lua_tonumber(L, 2);
		}
		if (args < 5) {
			sound->PlaySample(soundID, volume);
		}
		else {
			const float3 pos((float)lua_tonumber(L, 3),
			                 (float)lua_tonumber(L, 4),
			                 (float)lua_tonumber(L, 5));
			sound->PlaySample(soundID, pos, volume);
		}
		lua_pushboolean(L, 1);
	} else {
		lua_pushboolean(L, 0);
	}

	if (CLuaHandle::GetActiveHandle()->GetUserMode()) {
		return 1;
	} else {
		return 0;
	}
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::AddWorldIcon(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 4) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) ||
	    !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		luaL_error(L, "Incorrect arguments to AddWorldIcon(id, x, y, z");
	}
	const int cmdID = (int)lua_tonumber(L, 1);
	const float3 pos((float)lua_tonumber(L, 2),
	                 (float)lua_tonumber(L, 3),
	                 (float)lua_tonumber(L, 4));
	cursorIcons.AddIcon(cmdID, pos);
	return 0;
}


int LuaUnsyncedCtrl::AddWorldText(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 4) ||
	    !lua_isstring(L, 1) || !lua_isnumber(L, 2) ||
	    !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		luaL_error(L, "Incorrect arguments to AddWorldIcon(text, x, y, z");
	}
	const string text = lua_tostring(L, 1);
	const float3 pos((float)lua_tonumber(L, 2),
	                 (float)lua_tonumber(L, 3),
	                 (float)lua_tonumber(L, 4));
	cursorIcons.AddIconText(text, pos);
	return 0;
}


int LuaUnsyncedCtrl::AddWorldUnit(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 6) ||
	    !lua_isstring(L, 1) || !lua_isnumber(L, 2) ||
	    !lua_isnumber(L, 3) || !lua_isnumber(L, 4) ||
	    !lua_isnumber(L, 5) || !lua_isnumber(L, 6)) {
		luaL_error(L,
			"Incorrect arguments to AddWorldUnit(unitDefID, x, y, z, team, facing)");
	}
	const int unitDefID = (int)lua_tonumber(L, 1);
	if ((unitDefID < 0) || (unitDefID > unitDefHandler->numUnits)) {
		return 0;
	}
	const float3 pos((float)lua_tonumber(L, 2),
	                 (float)lua_tonumber(L, 3),
	                 (float)lua_tonumber(L, 4));
	const int team = (int)lua_tonumber(L, 5);
	if ((team < 0) || (team >= gs->activeTeams)) {
		return 0;
	}
	const int facing = (int)lua_tonumber(L, 6);
	cursorIcons.AddBuildIcon(-unitDefID, pos, team, facing);
	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::SetCameraTarget(lua_State* L)
{
	if (mouse == NULL) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args < 3) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
		luaL_error(L, "Incorrect arguments to SetCameraTarget(x, y, z)");
	}
	const float3 pos((float)lua_tonumber(L, 1),
	                 (float)lua_tonumber(L, 2),
	                 (float)lua_tonumber(L, 3));

	float transTime = 0.5f;
	if ((args >= 4) && lua_isnumber(L, 4)) {
		transTime = (float)lua_tonumber(L, 4);
	}

	mouse->currentCamController->SetPos(pos);
	mouse->CameraTransition(transTime);

	return 0;
}


int LuaUnsyncedCtrl::SetCameraState(lua_State* L)
{
	if (mouse == NULL) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_istable(L, 1) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetCameraState(table, camTime)");
	}

	const int table = 1;
	const float camTime = (float)lua_tonumber(L, 2);

	lua_pushstring(L, "mode");
	lua_gettable(L, table);
	if (lua_isnumber(L, -1)) {
		const int camNum = (int)lua_tonumber(L, -1);
		if ((camNum >= 0) && (camNum < mouse->camControllers.size())) {
			const int oldNum = mouse->currentCamControllerNum;
			const float3 dummy = mouse->currentCamController->SwitchFrom();
			mouse->currentCamControllerNum = camNum;
			mouse->currentCamController = mouse->camControllers[camNum];
			const bool showMode = (oldNum != camNum);
			mouse->currentCamController->SwitchTo(showMode);
			mouse->CameraTransition(camTime);
		}
	}

	vector<float> camState;
	int index = 1;
	while (true) {
		lua_rawgeti(L, table, index);
		if (!lua_isnumber(L, -1)) {
			lua_pop(L, 1);
			break;
		}
		else {
			camState.push_back((float)lua_tonumber(L, -1));
			lua_pop(L, 1);
			index++;
		}
	}

	lua_pushboolean(L, mouse->currentCamController->SetState(camState));

	if (CLuaHandle::GetActiveHandle()->GetUserMode()) {
		return 1;
	} else {
		return 0;
	}
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::SelectUnitArray(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_istable(L, 1) ||
	    ((args >= 2) && !lua_isboolean(L, 2))) {
		luaL_error(L, "Incorrect arguments to SelectUnitArray()");
	}

	// clear the current units, unless the append flag is present
	if ((args < 2) || !lua_toboolean(L, 2)) {
		selectedUnits.ClearSelected();
	}

	const int table = 1;
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_isnumber(L, -2) && lua_isnumber(L, -1)) {     // avoid 'n'
			CUnit* unit = ParseSelectUnit(L, __FUNCTION__, -1); // the value
			if (unit != NULL) {
				selectedUnits.AddUnit(unit);
			}
		}
	}
	return 0;
}


int LuaUnsyncedCtrl::SelectUnitMap(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_istable(L, 1) ||
	    ((args >= 2) && !lua_isboolean(L, 2))) {
		luaL_error(L, "Incorrect arguments to SelectUnitMap()");
	}

	// clear the current units, unless the append flag is present
	if ((args < 2) || !lua_toboolean(L, 2)) {
		selectedUnits.ClearSelected();
	}

	const int table = 1;
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_isnumber(L, -2)) {
			CUnit* unit = ParseSelectUnit(L, __FUNCTION__, -2); // the key
			if (unit != NULL) {
				selectedUnits.AddUnit(unit);
			}
		}
	}

	return 0;
}


/******************************************************************************/
/******************************************************************************/
