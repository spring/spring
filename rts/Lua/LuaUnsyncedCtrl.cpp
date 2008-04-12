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
#include "Game/Camera/CameraController.h"
#include "Game/CameraHandler.h"
#include "Game/Game.h"
#include "Game/SelectedUnits.h"
#include "Game/Team.h"
#include "Game/Player.h"
#include "Game/UI/CommandColors.h"
#include "Game/UI/CursorIcons.h"
#include "Game/UI/MouseHandler.h"
#include "Rendering/GL/myGL.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/LineDrawer.h"
#include "System/LogOutput.h"
#include "System/Sound.h"


/******************************************************************************/
/******************************************************************************/

CUnitSet LuaUnsyncedCtrl::drawCmdQueueUnits;

static const bool& fullRead     = CLuaHandle::GetActiveFullRead();
static const int&  readAllyTeam = CLuaHandle::GetActiveReadAllyTeam();


/******************************************************************************/
/******************************************************************************/

bool LuaUnsyncedCtrl::PushEntries(lua_State* L)
{
#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(Echo);

	REGISTER_LUA_CFUNC(SendMessage);
	REGISTER_LUA_CFUNC(SendMessageToPlayer);
	REGISTER_LUA_CFUNC(SendMessageToTeam);
	REGISTER_LUA_CFUNC(SendMessageToAllyTeam);
	REGISTER_LUA_CFUNC(SendMessageToSpectators);

	REGISTER_LUA_CFUNC(PlaySoundFile);
	REGISTER_LUA_CFUNC(PlaySoundStream);
	REGISTER_LUA_CFUNC(StopSoundStream);
	REGISTER_LUA_CFUNC(PauseSoundStream);
	REGISTER_LUA_CFUNC(GetSoundStreamTime);
	REGISTER_LUA_CFUNC(SetSoundStreamVolume);

	REGISTER_LUA_CFUNC(SetCameraState);
	REGISTER_LUA_CFUNC(SetCameraTarget);

	REGISTER_LUA_CFUNC(SelectUnitMap);
	REGISTER_LUA_CFUNC(SelectUnitArray);

	REGISTER_LUA_CFUNC(AddWorldIcon);
	REGISTER_LUA_CFUNC(AddWorldText);
	REGISTER_LUA_CFUNC(AddWorldUnit);

	REGISTER_LUA_CFUNC(DrawUnitCommands);

	REGISTER_LUA_CFUNC(SetTeamColor);

	REGISTER_LUA_CFUNC(AssignMouseCursor);
	REGISTER_LUA_CFUNC(ReplaceMouseCursor);

	REGISTER_LUA_CFUNC(SetCustomCommandDrawData);

	REGISTER_LUA_CFUNC(SetDrawSky);
	REGISTER_LUA_CFUNC(SetDrawWater);
	REGISTER_LUA_CFUNC(SetDrawGround);

	REGISTER_LUA_CFUNC(SetUnitNoDraw);
	REGISTER_LUA_CFUNC(SetUnitNoMinimap);
	REGISTER_LUA_CFUNC(SetUnitNoSelect);
            
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

static inline CUnit* ParseRawUnit(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		if (caller != NULL) {
			luaL_error(L, "Bad unitID parameter in %s()\n", caller);
		} else {
			return NULL;
		}
	}
	const int unitID = (int)lua_tonumber(L, index);
	if ((unitID < 0) || (unitID >= MAX_UNITS)) {
		luaL_error(L, "%s(): Bad unitID: %i\n", caller, unitID);
	}
	CUnit* unit = uh->units[unitID];
	if (unit == NULL) {
		return NULL;
	}
	return unit;
}


static inline CUnit* ParseAllyUnit(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseRawUnit(L, caller, index);
	if (unit == NULL) {
		return NULL;
	}
	if (readAllyTeam < 0) {
		return fullRead ? unit : NULL;
	}
	return (unit->allyteam == readAllyTeam) ? unit : NULL;
}


static inline CUnit* ParseCtrlUnit(lua_State* L,
                                     const char* caller, int index)
{
	CUnit* unit = ParseRawUnit(L, caller, index);
	if (unit == NULL) {
		return NULL;
	}
	return (CanCtrlTeam(unit->team) ? unit : NULL);
}


static inline CUnit* ParseSelectUnit(lua_State* L,
                                     const char* caller, int index)
{
	CUnit* unit = ParseRawUnit(L, caller, index);
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

void LuaUnsyncedCtrl::DrawUnitCommandQueues()
{
	if (drawCmdQueueUnits.empty()) {
		return;
	}

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);

	lineDrawer.Configure(cmdColors.UseColorRestarts(),
	                     cmdColors.UseRestartColor(),
	                     cmdColors.restart,
	                     cmdColors.RestartAlpha());
	lineDrawer.SetupLineStipple();

	glEnable(GL_BLEND);
	glBlendFunc((GLenum)cmdColors.QueuedBlendSrc(),
	            (GLenum)cmdColors.QueuedBlendDst());

	glLineWidth(cmdColors.QueuedLineWidth());

	const CUnitSet& units = drawCmdQueueUnits;
	CUnitSet::const_iterator ui;
	for (ui = units.begin(); ui != units.end(); ++ui) {
		CUnit* unit = *ui;
		if (unit) {
			unit->commandAI->DrawCommands();
		}
	}

	glLineWidth(1.0f);

	glEnable(GL_DEPTH_TEST);
}


void LuaUnsyncedCtrl::ClearUnitCommandQueues()
{
	drawCmdQueueUnits.clear();
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
		if (lua_israwnumber(L, -2)) {  // only numeric keys
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


static string ParseMessage(lua_State* L, const string& msg)
{
	string::size_type start = msg.find("<PLAYER");
	if (start == string::npos) {
		return msg;
	}

	const char* number = msg.c_str() + start + strlen("<PLAYER");
	char* endPtr;
	const int playerID = (int)strtol(number, &endPtr, 10);
	if ((endPtr == number) || (*endPtr != '>')) {
		luaL_error(L, "Bad message format: %s", msg.c_str());
	}

	if ((playerID < 0) || (playerID >= MAX_PLAYERS)) {
		luaL_error(L, "Invalid message playerID: %i", playerID);
	}
	const CPlayer* player = gs->players[playerID];
	if ((player == NULL) || !player->active || player->playerName.empty()) {
		luaL_error(L, "Invalid message playerID: %i", playerID);
	}

	const string head = msg.substr(0, start);
	const string tail = msg.substr(endPtr - msg.c_str() + 1);

	return head + player->playerName + ParseMessage(L, tail);
}


static void PrintMessage(lua_State* L, const string& msg)
{
	logOutput.Print(ParseMessage(L, msg));
}


int LuaUnsyncedCtrl::SendMessage(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to SendMessage()");
	}
	PrintMessage(L, lua_tostring(L, 1));
	return 0;
}


int LuaUnsyncedCtrl::SendMessageToSpectators(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to SendMessageToSpectators()");
	}
	if (gu->spectating) {
		PrintMessage(L, lua_tostring(L, 1));
	}
	return 0;
}


int LuaUnsyncedCtrl::SendMessageToPlayer(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
		luaL_error(L, "Incorrect arguments to SendMessageToPlayer()");
	}
	const int playerID = (int)lua_tonumber(L, 1);
	if (playerID == gu->myPlayerNum) {
		PrintMessage(L, lua_tostring(L, 2));
	}
	return 0;
}


int LuaUnsyncedCtrl::SendMessageToTeam(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
		luaL_error(L, "Incorrect arguments to SendMessageToTeam()");
	}
	const int teamID = (int)lua_tonumber(L, 1);
	if (teamID == gu->myTeam) {
		PrintMessage(L, lua_tostring(L, 2));
	}
	return 0;
}


int LuaUnsyncedCtrl::SendMessageToAllyTeam(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
		luaL_error(L, "Incorrect arguments to SendMessageToAllyTeam()");
	}
	const int allyTeamID = (int)lua_tonumber(L, 1);
	if (allyTeamID == gu->myAllyTeam) {
		PrintMessage(L, lua_tostring(L, 2));
	}
	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::PlaySoundFile(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to PlaySoundFile()");
	}
	bool success = false;
	const string soundFile = lua_tostring(L, 1);
	const unsigned int soundID = sound->GetWaveId(soundFile, false);
	if (soundID > 0) {
		float volume = 1.0f;
		if (args >= 2) {
			volume = (float)lua_tonumber(L, 2);
		}

		if (args < 5) {
			sound->PlaySample(soundID, volume);
		} else {
			const float3 pos((float)lua_tonumber(L, 3),
			                 (float)lua_tonumber(L, 4),
			                 (float)lua_tonumber(L, 5));
			sound->PlaySample(soundID, pos, volume);
		}
		success = true;
	}

	if (CLuaHandle::GetActiveHandle()->GetUserMode()) {
		lua_pushboolean(L, success);
		return 1;
	} else {
		return 0;
	}
}


int LuaUnsyncedCtrl::PlaySoundStream(lua_State* L)
{
	const int args = lua_gettop(L);

	const string soundFile = luaL_checkstring(L, 1);
	const float volume     =   luaL_optnumber(L, 2, 1.0f);

	if (args < 5) {
		sound->PlayStream(soundFile, volume);
	}
	else {
		const float3 pos((float) lua_tonumber(L, 3),
		                 (float) lua_tonumber(L, 4),
		                 (float) lua_tonumber(L, 5));
		sound->PlayStream(soundFile, volume, pos);
	}

	// .ogg files don't have sound ID's generated
	// for them (yet), so we always succeed here
	if (CLuaHandle::GetActiveHandle()->GetUserMode()) {
		lua_pushboolean(L, true);
		return 1;
	} else {
		return 0;
	}
}

int LuaUnsyncedCtrl::StopSoundStream(lua_State*)
{
	sound->StopStream();
	return 0;
}
int LuaUnsyncedCtrl::PauseSoundStream(lua_State*)
{
	sound->PauseStream();
	return 0;
}
int LuaUnsyncedCtrl::GetSoundStreamTime(lua_State* L)
{
	lua_pushnumber(L, sound->GetStreamTime());
	return 1;
}
int LuaUnsyncedCtrl::SetSoundStreamVolume(lua_State* L)
{
	const int args = lua_gettop(L);
	if (args == 1) {
		sound->SetStreamVolume(lua_tonumber(L, 1));
	} else {
		luaL_error(L, "Incorrect arguments to SetSoundStreamVolume(v)");
	}
	return 0;
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
	if ((unitDefID < 0) || (unitDefID > unitDefHandler->numUnitDefs)) {
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


int LuaUnsyncedCtrl::DrawUnitCommands(lua_State* L)
{
	if (lua_istable(L, 1)) {
		const bool isMap = lua_isboolean(L, 2) && lua_toboolean(L, 2);
		const int unitArg = isMap ? -2 : -1;
		const int table = 1;
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (lua_israwnumber(L, -2)) {
				CUnit* unit = ParseAllyUnit(L, __FUNCTION__, unitArg);
				if (unit != NULL) {
					drawCmdQueueUnits.insert(unit);
				}
			}
		}
		return 0;
	}
	CUnit* unit = ParseAllyUnit(L, __FUNCTION__, 1);
	if (unit != NULL) {
		drawCmdQueueUnits.insert(unit);
	}
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

	camHandler->GetCurrentController().SetPos(pos);
	camHandler->CameraTransition(transTime);

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
		camHandler->SetCameraMode(camNum);
		camHandler->CameraTransition(camTime);
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

	lua_pushboolean(L, camHandler->GetCurrentController().SetState(camState));
	camHandler->CameraTransition(camTime);

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
		if (lua_israwnumber(L, -2) && lua_isnumber(L, -1)) {     // avoid 'n'
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
		if (lua_israwnumber(L, -2)) {
			CUnit* unit = ParseSelectUnit(L, __FUNCTION__, -2); // the key
			if (unit != NULL) {
				selectedUnits.AddUnit(unit);
			}
		}
	}

	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::SetTeamColor(lua_State* L)
{
	// FIXME: doesn't work for 3DO team textures
	//        doesn't play nicely with cached team color (in scripts, etc...)
	const int teamID = (int)luaL_checknumber(L, 1);
	if ((teamID < 0) || (teamID >= MAX_TEAMS)) {
		return 0;
	}
	CTeam* team = gs->Team(teamID);
	if (team == NULL) {
		return 0;
	}
	const float r = max(0.0f, min(1.0f, luaL_checknumber(L, 2)));
	const float g = max(0.0f, min(1.0f, luaL_checknumber(L, 3)));
	const float b = max(0.0f, min(1.0f, luaL_checknumber(L, 4)));
	team->color[0] = (unsigned char)(r * 255.0f);
	team->color[1] = (unsigned char)(g * 255.0f);
	team->color[2] = (unsigned char)(b * 255.0f);
	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::AssignMouseCursor(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isstring(L, 1) || !lua_isstring(L, 2)) {
		luaL_error(L, "Incorrect arguments to AssignMouseCursor()");
	}

	const string cmdName  = lua_tostring(L, 1);
	const string fileName = lua_tostring(L, 2);

	bool overwrite = true;
	if ((args >= 3) && lua_isboolean(L, 3)) {
		overwrite = lua_toboolean(L, 3);
	}

	CMouseCursor::HotSpot hotSpot = CMouseCursor::Center;
	if ((args >= 4) && lua_isboolean(L, 4)) {
		if (lua_toboolean(L, 4)) {
			hotSpot = CMouseCursor::TopLeft;
		}
	}

	if (mouse->AssignMouseCursor(cmdName, fileName, hotSpot, overwrite)) {
		lua_pushboolean(L, true);
	} else {
		lua_pushboolean(L, false);
	}
	return 1;
}


int LuaUnsyncedCtrl::ReplaceMouseCursor(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1) || !lua_isstring(L, 2)) {
		luaL_error(L, "Incorrect arguments to ReplaceMouseCursor()");
	}

	const string oldName = lua_tostring(L, 1);
	const string newName = lua_tostring(L, 2);

	CMouseCursor::HotSpot hotSpot = CMouseCursor::Center;
	if ((args >= 3) && lua_isboolean(L, 3)) {
		if (lua_toboolean(L, 3)) {
			hotSpot = CMouseCursor::TopLeft;
		}
	}

	lua_pushboolean(L, mouse->ReplaceMouseCursor(oldName, newName, hotSpot));
	return 1;
}

/******************************************************************************/

int LuaUnsyncedCtrl::SetCustomCommandDrawData(lua_State* L)
{
	const int cmdID = (int)luaL_checknumber(L, 1);

	int iconID = 0;
	if (lua_israwnumber(L, 2)) {
		iconID = (int)lua_tonumber(L, 2);
	}
	else if (lua_israwstring(L, 2)) {
		iconID = cmdID;
		const string icon = lua_tostring(L, 2);
		cursorIcons.SetCustomType(cmdID, icon);
	}
	else if (lua_isnil(L, 2)) {
		cursorIcons.SetCustomType(cmdID, "");
		cmdColors.ClearCustomCmdData(cmdID);
		return 0;
	}
	else {
		luaL_error(L, "Incorrect arguments to SetCustomCommandDrawData");
	}
	
	float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	const int table = 3;
	if (lua_istable(L, table)) {
		for (int i = 0; i < 4; i++) {
			lua_rawgeti(L, table, i + 1);
			if (lua_israwnumber(L, -1)) {
				color[i] = (float)lua_tonumber(L, -1);
				lua_pop(L, 1);
			} else {
				lua_pop(L, 1);
				break;
			}
		}
	}

	const bool showArea = lua_isboolean(L, 4) && lua_toboolean(L, 4);

	cmdColors.SetCustomCmdData(cmdID, iconID, color, showArea);

	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::SetDrawSky(lua_State* L)
{
	if (game == NULL) {
		return 0;
	}
	if (!lua_isboolean(L, 1)) {
		luaL_error(L, "Incorrect arguments to SetDrawSky()");
	}
	game->drawSky = !!lua_toboolean(L, 1);
	return 0;
}


int LuaUnsyncedCtrl::SetDrawWater(lua_State* L)
{
	if (game == NULL) {
		return 0;
	}
	if (!lua_isboolean(L, 1)) {
		luaL_error(L, "Incorrect arguments to SetDrawWater()");
	}
	game->drawWater = !!lua_toboolean(L, 1);
	return 0;
}


int LuaUnsyncedCtrl::SetDrawGround(lua_State* L)
{
	if (game == NULL) {
		return 0;
	}
	if (!lua_isboolean(L, 1)) {
		luaL_error(L, "Incorrect arguments to SetDrawGround()");
	}
	game->drawGround = !!lua_toboolean(L, 1);
	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::SetUnitNoDraw(lua_State* L)
{
	if (CLuaHandle::GetActiveHandle()->GetUserMode()) {
		return 0;
	}
	CUnit* unit = ParseCtrlUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isboolean(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetUnitNoDraw()");
	}
	unit->noDraw = lua_toboolean(L, 2);
	return 0;
}


int LuaUnsyncedCtrl::SetUnitNoMinimap(lua_State* L)
{
	if (CLuaHandle::GetActiveHandle()->GetUserMode()) {
		return 0;
	}
	CUnit* unit = ParseCtrlUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isboolean(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetUnitNoMinimap()");
	}
	unit->noMinimap = lua_toboolean(L, 2);
	return 0;
}


int LuaUnsyncedCtrl::SetUnitNoSelect(lua_State* L)
{
	if (CLuaHandle::GetActiveHandle()->GetUserMode()) {
		return 0;
	}
	CUnit* unit = ParseCtrlUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isboolean(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetUnitNoSelect()");
	}
	unit->noSelect = lua_toboolean(L, 2);

	// deselect the unit if it's selected and shouldn't be
	if (unit->noSelect) {
		PUSH_CODE_MODE;
		ENTER_MIXED;
		const CUnitSet& selUnits = selectedUnits.selectedUnits;
		if (selUnits.find(unit) != selUnits.end()) {
			selectedUnits.RemoveUnit(unit);
		}
		POP_CODE_MODE;
	}
	return 0;
}


/******************************************************************************/
/******************************************************************************/
