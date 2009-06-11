#include "StdAfx.h"
// LuaUnsyncedCtrl.cpp: implementation of the LuaUnsyncedCtrl class.
//
//////////////////////////////////////////////////////////////////////

#include <set>
#include <list>
#include <cctype>

#include <fstream>

#include <SDL_keysym.h>
#include <SDL_mouse.h>
#include <SDL_timer.h>

#include "mmgr.h"

#include "LuaUnsyncedCtrl.h"

#include "LuaInclude.h"

#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaUtils.h"
#include "LuaTextures.h"

#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/Camera/CameraController.h"
#include "Game/Game.h"
#include "Game/SelectedUnits.h"
#include "Game/PlayerHandler.h"
#include "Game/UI/CommandColors.h"
#include "Game/UI/CursorIcons.h"
#include "Game/UI/GuiHandler.h"
#include "Game/UI/InfoConsole.h"
#include "Game/UI/KeyCodes.h"
#include "Game/UI/KeySet.h"
#include "Game/UI/KeyBindings.h"
#include "Game/UI/MiniMap.h"
#include "Game/UI/MouseHandler.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Map/BaseGroundDrawer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/FontTexture.h"
#include "Rendering/IconHandler.h"
#include "Rendering/InMapDraw.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/LineDrawer.h"
#include "Sim/Units/Groups/Group.h"
#include "Sim/Units/Groups/GroupHandler.h"
#include "LogOutput.h"
#include "NetProtocol.h"
#include "Sound/Sound.h"
#include "Sound/AudioChannel.h"
#include "Sound/Music.h"

#include "FileSystem/FileHandler.h"
#include "FileSystem/FileSystem.h"
#include "ConfigHandler.h"

#include <boost/cstdint.hpp>

using namespace std;

// MinGW defines this for a WINAPI function
#undef SendMessage
extern boost::uint8_t *keys;

const int CMD_INDEX_OFFSET = 1; // starting index for command descriptions


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

	REGISTER_LUA_CFUNC(LoadSoundDef);
	REGISTER_LUA_CFUNC(PlaySoundFile);
	REGISTER_LUA_CFUNC(PlaySoundStream);
	REGISTER_LUA_CFUNC(StopSoundStream);
	REGISTER_LUA_CFUNC(PauseSoundStream);
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

	REGISTER_LUA_CFUNC(SetWaterParams);

	REGISTER_LUA_CFUNC(SetUnitNoDraw);
	REGISTER_LUA_CFUNC(SetUnitNoMinimap);
	REGISTER_LUA_CFUNC(SetUnitNoSelect);

	REGISTER_LUA_CFUNC(AddUnitIcon);
	REGISTER_LUA_CFUNC(FreeUnitIcon);

	REGISTER_LUA_CFUNC(ExtractModArchiveFile);

	// moved from LuaUI

//FIXME	REGISTER_LUA_CFUNC(SetShockFrontFactors);

	REGISTER_LUA_CFUNC(GetConfigInt);
	REGISTER_LUA_CFUNC(SetConfigInt);
	REGISTER_LUA_CFUNC(GetConfigString);
	REGISTER_LUA_CFUNC(SetConfigString);

	REGISTER_LUA_CFUNC(CreateDir);
	REGISTER_LUA_CFUNC(MakeFont);

	REGISTER_LUA_CFUNC(SendCommands);
	REGISTER_LUA_CFUNC(GiveOrder);
	REGISTER_LUA_CFUNC(GiveOrderToUnit);
	REGISTER_LUA_CFUNC(GiveOrderToUnitMap);
	REGISTER_LUA_CFUNC(GiveOrderToUnitArray);
	REGISTER_LUA_CFUNC(GiveOrderArrayToUnitMap);
	REGISTER_LUA_CFUNC(GiveOrderArrayToUnitArray);

	REGISTER_LUA_CFUNC(SendLuaUIMsg);
	REGISTER_LUA_CFUNC(SendLuaGaiaMsg);
	REGISTER_LUA_CFUNC(SendLuaRulesMsg);

	REGISTER_LUA_CFUNC(SetActiveCommand);

	REGISTER_LUA_CFUNC(ForceLayoutUpdate);

	REGISTER_LUA_CFUNC(SetMouseCursor);
	REGISTER_LUA_CFUNC(WarpMouse);

	REGISTER_LUA_CFUNC(SetCameraOffset);

	REGISTER_LUA_CFUNC(SetLosViewColors);

	REGISTER_LUA_CFUNC(SetUnitDefIcon);
	REGISTER_LUA_CFUNC(SetUnitDefImage);

	REGISTER_LUA_CFUNC(SetUnitGroup);

	REGISTER_LUA_CFUNC(SetShareLevel);
	REGISTER_LUA_CFUNC(ShareResources);

	REGISTER_LUA_CFUNC(SetLastMessagePosition);

	REGISTER_LUA_CFUNC(MarkerAddPoint);
	REGISTER_LUA_CFUNC(MarkerAddLine);
	REGISTER_LUA_CFUNC(MarkerErasePosition);

	REGISTER_LUA_CFUNC(SetDrawSelectionInfo);

	REGISTER_LUA_CFUNC(SetBuildSpacing);
	REGISTER_LUA_CFUNC(SetBuildFacing);

	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Access helpers
//

static inline void CheckNoArgs(lua_State* L, const char* funcName)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		luaL_error(L, "%s() takes no arguments", funcName);
	}
}


static inline bool CheckModUICtrl()
{
	return CLuaHandle::GetModUICtrl() ||
	       CLuaHandle::GetActiveHandle()->GetUserMode();
}


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
	return teamHandler->AllyTeam(ctrlTeam);
}


static inline bool CanCtrlTeam(int team)
{
	const int ctrlTeam = CtrlTeam();
	if (ctrlTeam < 0) {
		return (ctrlTeam == CEventClient::AllAccessTeam) ? true : false;
	}
	return (ctrlTeam == team);
}


static inline bool CanCtrlAllyTeam(int allyteam)
{
	const int ctrlTeam = CtrlTeam();
	if (ctrlTeam < 0) {
		return (ctrlTeam == CEventClient::AllAccessTeam) ? true : false;
	}
	return (teamHandler->AllyTeam(ctrlTeam) == allyteam);
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
	const int unitID = lua_toint(L, index);
	if ((unitID < 0) || (static_cast<size_t>(unitID) >= uh->MaxUnits())) {
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
	if (unit == NULL || unit->noSelect) {
		return NULL;
	}
	const int selectTeam = CLuaHandle::GetActiveHandle()->GetSelectTeam();
	if (selectTeam < 0) {
		return (selectTeam == CEventClient::AllAccessTeam) ? unit : NULL;
	}
	if (selectTeam == unit->team) {
		return unit;
	}
	return NULL;
}


static int ParseFloatArray(lua_State* L, int index, float* array, int size)
{
	if (!lua_istable(L, index)) {
		return -1;
	}
	const int table = (index > 0) ? index : (lua_gettop(L) + index + 1);
	for (int i = 0; i < size; i++) {
		lua_rawgeti(L, table, (i + 1));
		if (lua_isnumber(L, -1)) {
			array[i] = lua_tofloat(L, -1);
			lua_pop(L, 1);
		} else {
			lua_pop(L, 1);
			return i;
		}
	}
	return size;
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

	GML_STDMUTEX_LOCK(cai); // DrawUnitCommandQueues
	GML_STDMUTEX_LOCK(dque); // DrawUnitCommandQueues

	const CUnitSet& units = drawCmdQueueUnits;
	CUnitSet::const_iterator ui;
	for (ui = units.begin(); ui != units.end(); ++ui) {
		CUnit* unit = *ui;
		if (unit) {
			CCommandAI *cai=unit->commandAI;
			if(cai)
				cai->DrawCommands();
		}
	}

	glLineWidth(1.0f);

	glEnable(GL_DEPTH_TEST);
}


void LuaUnsyncedCtrl::ClearUnitCommandQueues()
{
	GML_STDMUTEX_LOCK(dque); // ClearUnitCommandQueues

	drawCmdQueueUnits.clear();
}


/******************************************************************************/
/******************************************************************************/
//
//  The call-outs
//

int LuaUnsyncedCtrl::Echo(lua_State* L)
{
	return LuaUtils::Echo(L);
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

	if ((playerID < 0) || (playerID >= playerHandler->ActivePlayers())) {
		luaL_error(L, "Invalid message playerID: %i", playerID);
	}
	const CPlayer* player = playerHandler->Player(playerID);
	if ((player == NULL) || !player->active || player->name.empty()) {
		luaL_error(L, "Invalid message playerID: %i", playerID);
	}

	const string head = msg.substr(0, start);
	const string tail = msg.substr(endPtr - msg.c_str() + 1);

	return head + player->name + ParseMessage(L, tail);
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
	const int playerID = lua_toint(L, 1);
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
	const int teamID = lua_toint(L, 1);
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
	const int allyTeamID = lua_toint(L, 1);
	if (allyTeamID == gu->myAllyTeam) {
		PrintMessage(L, lua_tostring(L, 2));
	}
	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::LoadSoundDef(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to LoadSoundDef()");
	}

	const string soundFile = lua_tostring(L, 1);
	bool success = sound->LoadSoundDefs(soundFile);

	if (CLuaHandle::GetActiveHandle()->GetUserMode()) {
		lua_pushboolean(L, success);
		return 1;
	} else {
		return 0;
	}
}

int LuaUnsyncedCtrl::PlaySoundFile(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to PlaySoundFile()");
	}
	bool success = false;
	const string soundFile = lua_tostring(L, 1);
	const unsigned int soundID = sound->GetSoundId(soundFile, false);
	if (soundID > 0) {
		float volume = 1.0f;
		if (args >= 2) {
			volume = lua_tofloat(L, 2);
		}

		if (args < 5) {
			Channels::General.PlaySample(soundID, volume);
		} else {
			const float3 pos(lua_tofloat(L, 3),
			                 lua_tofloat(L, 4),
			                 lua_tofloat(L, 5));
			if (args >= 8)
			{
				const float3 speed(lua_tofloat(L, 6), lua_tofloat(L, 7), lua_tofloat(L, 8));
				Channels::General.PlaySample(soundID, pos, speed, volume);
			}
			else
				Channels::General.PlaySample(soundID, pos, volume);
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
	//const int args = lua_gettop(L);

	const string soundFile = luaL_checkstring(L, 1);
	const float volume = luaL_optnumber(L, 2, 1.0f);

	Channels::BGMusic.Play(soundFile, volume);

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
	Channels::BGMusic.Stop();
	return 0;
}
int LuaUnsyncedCtrl::PauseSoundStream(lua_State*)
{
	Channels::BGMusic.Pause();
	return 0;
}
int LuaUnsyncedCtrl::SetSoundStreamVolume(lua_State* L)
{
	const int args = lua_gettop(L);
	if (args == 1) {
		Channels::BGMusic.SetVolume(lua_tonumber(L, 1));
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
	const int cmdID = lua_toint(L, 1);
	const float3 pos(lua_tofloat(L, 2),
	                 lua_tofloat(L, 3),
	                 lua_tofloat(L, 4));
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
	const float3 pos(lua_tofloat(L, 2),
	                 lua_tofloat(L, 3),
	                 lua_tofloat(L, 4));
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
	const int unitDefID = lua_toint(L, 1);
	if ((unitDefID < 0) || (unitDefID > unitDefHandler->numUnitDefs)) {
		return 0;
	}
	const float3 pos(lua_tofloat(L, 2),
	                 lua_tofloat(L, 3),
	                 lua_tofloat(L, 4));
	const int team = lua_toint(L, 5);
	if ((team < 0) || (team >= teamHandler->ActiveTeams())) {
		return 0;
	}
	const int facing = lua_toint(L, 6);
	cursorIcons.AddBuildIcon(-unitDefID, pos, team, facing);
	return 0;
}


int LuaUnsyncedCtrl::DrawUnitCommands(lua_State* L)
{
	GML_STDMUTEX_LOCK(dque); // DrawUnitCommands

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
	const float3 pos(lua_tofloat(L, 1),
	                 lua_tofloat(L, 2),
	                 lua_tofloat(L, 3));

	float transTime = 0.5f;
	if ((args >= 4) && lua_isnumber(L, 4)) {
		transTime = lua_tofloat(L, 4);
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

	if (!lua_istable(L, 1)) {
		luaL_error(L, "Incorrect arguments to SetCameraState(table, camTime)");
	}

	const float camTime = luaL_checkfloat(L, 2);

	CCameraController::StateMap camState;

	const int table = 1;
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_israwstring(L, -2)) {
			const string key = lua_tostring(L, -2);
			if (lua_isnumber(L, -1)) {
				camState[key] = lua_tofloat(L, -1);
			}
			else if (lua_isboolean(L, -1)) {
				camState[key] = lua_toboolean(L, -1) ? +1.0f : -1.0f;
			}
		}
	}

	lua_pushboolean(L, camHandler->SetState(camState));
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
	const int teamID = luaL_checkint(L, 1);
	if ((teamID < 0) || (teamID >= teamHandler->ActiveTeams())) {
		return 0;
	}
	CTeam* team = teamHandler->Team(teamID);
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
	if (!CLuaHandle::GetActiveHandle()->GetUserMode()) {
		return 0;
	}

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
	if (!CLuaHandle::GetActiveHandle()->GetUserMode()) {
		return 0;
	}

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
	const int cmdID = luaL_checkint(L, 1);

	int iconID = 0;
	if (lua_israwnumber(L, 2)) {
		iconID = lua_toint(L, 2);
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
				color[i] = lua_tofloat(L, -1);
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

int LuaUnsyncedCtrl::SetWaterParams(lua_State* L)
{
	if (game == NULL) {
		return 0;
	}
	if (!gs->cheatEnabled) {
		logOutput.Print("SetWaterParams() needs cheating enabled");
		return 0;
	}
	if (!lua_istable(L, 1)) {
		luaL_error(L, "Incorrect arguments to SetWaterParams()");
	}

	CMapInfo::water_t& w = const_cast<CMapInfo*>(mapInfo)->water;
	for (lua_pushnil(L); lua_next(L, 1) != 0; lua_pop(L, 1)) {
		if (lua_israwstring(L, -2)) {
			const string key = lua_tostring(L, -2);
			if (lua_istable(L, -1)) {
				float color[3];
				const int size = ParseFloatArray(L, -1, color, 3);
				if (size>=3) {
					if (key == "absorb") {
						w.absorb = color;
					} else if (key == "baseColor") {
						w.baseColor = color;
					} else if (key == "minColor") {
						w.minColor = color;
					} else if (key == "surfaceColor") {
						w.surfaceColor = color;
					} else if (key == "diffuseColor") {
						w.diffuseColor = color;
					} else if (key == "specularColor") {
						w.specularColor = color;
 					} else if (key == "planeColor") {
						w.planeColor = color;
					}
				}
			}
			else if (lua_israwstring(L, -1)) {
				const std::string value = lua_tostring(L, -1);
				if (key == "texture") {
					w.texture = value;
				} else if (key == "foamTexture") {
					w.foamTexture = value;
				} else if (key == "normalTexture") {
					w.normalTexture = value;
				}
			}
			else if (lua_isnumber(L, -1)) {
				const float value = lua_tonumber(L, -1);
				if (key == "damage") {
					w.damage = value;
				} else if (key == "repeatX") {
					w.repeatX = value;
				} else if (key == "repeatY") {
					w.repeatY = value;
				} else if (key == "surfaceAlpha") {
					w.surfaceAlpha = value;
				} else if (key == "ambientFactor") {
					w.ambientFactor = value;
				} else if (key == "diffuseFactor") {
					w.diffuseFactor = value;
				} else if (key == "specularFactor") {
					w.specularFactor = value;
				} else if (key == "specularPower") {
					w.specularPower = value;
				} else if (key == "fresnelMin") {
					w.fresnelMin = value;
				} else if (key == "fresnelMax") {
					w.fresnelMax = value;
				} else if (key == "fresnelPower") {
					w.fresnelPower = value;
				} else if (key == "reflectionDistortion") {
					w.reflDistortion = value;
				} else if (key == "blurBase") {
					w.blurBase = value;
				} else if (key == "blurExponent") {
					w.blurExponent = value;
				} else if (key == "perlinStartFreq") {
					w.perlinStartFreq = value;
				} else if (key == "perlinLacunarity") {
					w.perlinLacunarity = value;
				} else if (key == "perlinAmplitude") {
					w.perlinAmplitude = value;
				} else if (key == "numTiles") {
					w.numTiles = (unsigned char)value;
				}
			}
			else if (lua_isboolean(L, -1)) {
				const bool value = lua_toboolean(L, -1);
				if (key == "shoreWaves") {
					w.shoreWaves = value;
				} else if (key == "forceRendering") {
					w.forceRendering = value;
				} else if (key == "hasWaterPlane") {
					w.hasWaterPlane = value;
				}
			}
		}
	}

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
	GML_RECMUTEX_LOCK(sel); // SetUnitNoSelect

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
		const CUnitSet& selUnits = selectedUnits.selectedUnits;
		if (selUnits.find(unit) != selUnits.end()) {
			selectedUnits.RemoveUnit(unit);
		}
	}
	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::AddUnitIcon(lua_State* L)
{
	if (!CLuaHandle::GetActiveHandle()->GetUserMode()) {
		return 0;
	}
	const string iconName  = luaL_checkstring(L, 1);
	const string texName   = luaL_checkstring(L, 2);
	const float  size      = luaL_optnumber(L, 3, 1.0f);
	const float  dist      = luaL_optnumber(L, 4, 1.0f);
	const bool   radAdjust = (lua_isboolean(L, 5) && lua_toboolean(L, 5));
	lua_pushboolean(L, iconHandler->AddIcon(iconName, texName,
	                                        size, dist, radAdjust));
	return 1;
}


int LuaUnsyncedCtrl::FreeUnitIcon(lua_State* L)
{
	if (!CLuaHandle::GetActiveHandle()->GetUserMode()) {
		return 0;
	}
	const string iconName  = luaL_checkstring(L, 1);
	lua_pushboolean(L, iconHandler->FreeIcon(iconName));
	return 1;
}


/******************************************************************************/

// TODO: move this to LuaVFS?
int LuaUnsyncedCtrl::ExtractModArchiveFile(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}

	const string path = luaL_checkstring(L, 1);

	CFileHandler fh(path, SPRING_VFS_MOD);

	if (!fh.FileExists()) {
		luaL_error(L, "Path \"%s\" not found in mod archive", path.c_str());
	}

	string dname = filesystem.GetDirectory(path);
	string fname = filesystem.GetFilename(path);

#ifdef WIN32
	const int s = dname.size();
	// get rid of any trailing slashes (CreateDirectory()
	// fails on at least XP and Vista if they are present,
	// ie. it creates the dir but actually returns false)
	if ((s > 0) && ((dname[s - 1] == '/') || (dname[s - 1] == '\\'))) {
		dname = dname.substr(0, s - 1);
	}
#endif

	if (!dname.empty() && !filesystem.CreateDirectory(dname)) {
		luaL_error(L, "Could not create directory \"%s\" for file \"%s\"",
		           dname.c_str(), fname.c_str());
	}

	const int numBytes = fh.FileSize();
	char* buffer = new char[numBytes];

	fh.Read(buffer, numBytes);

	fstream fstr(path.c_str(), ios::out | ios::binary);
	fstr.write((const char*) buffer, numBytes);
	fstr.close();

	if (!dname.empty()) {
		logOutput.Print("Extracted file \"%s\" to directory \"%s\"",
		                fname.c_str(), dname.c_str());
	} else {
		logOutput.Print("Extracted file \"%s\"", fname.c_str());
	}

	delete[] buffer;

	lua_pushboolean(L, true);

	return 1;
}


/******************************************************************************/
/******************************************************************************/
//
// moved from LuaUI
//
/******************************************************************************/
/******************************************************************************/


int LuaUnsyncedCtrl::SendCommands(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	if ((guihandler == NULL) || gs->noHelperAIs) {
		return 0;
	}

	vector<string> cmds;

	if (lua_istable(L, 1)) { // old style -- table
		const int table = 1;
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (lua_israwstring(L, -1)) {
				string action = lua_tostring(L, -1);
				if (action[0] != '@') {
					action = "@@" + action;
				}
				cmds.push_back(action);
			}
		}
	}
	else if (lua_israwstring(L, 1)) { // new style -- function parameters
		for (int i = 1; lua_israwstring(L, i); i++) {
			string action = lua_tostring(L, i);
			if (action[0] != '@') {
				action = "@@" + action;
			}
			cmds.push_back(action);
		}
	}
	else {
		luaL_error(L, "Incorrect arguments to SendCommands()");
	}

	lua_settop(L, 0); // pop the input arguments

	guihandler->RunCustomCommands(cmds, false);

	return 0;
}


/******************************************************************************/

static int SetActiveCommandByIndex(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	if (guihandler == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	const int cmdIndex = lua_toint(L, 1) - CMD_INDEX_OFFSET;
	int button = 1; // LMB
	if ((args >= 2) && lua_isnumber(L, 2)) {
		button = lua_toint(L, 2);
	}

	if (args <= 2) {
		const bool rmb = (button == SDL_BUTTON_LEFT) ? false : true;
		const bool success = guihandler->SetActiveCommand(cmdIndex, rmb);
		lua_pushboolean(L, success);
		return 1;
	}

	// cmdIndex, button, lmb, rmb, alt, ctrl, meta, shift
	if ((args < 8) ||
	    !lua_isboolean(L, 3) || !lua_isboolean(L, 4) || !lua_isboolean(L, 5) ||
	    !lua_isboolean(L, 6) || !lua_isboolean(L, 7) || !lua_isboolean(L, 8)) {
		lua_pushstring(L, "Incorrect arguments to SetActiveCommand()");
	}
	const bool lmb   = lua_toboolean(L, 3);
	const bool rmb   = lua_toboolean(L, 4);
	const bool alt   = lua_toboolean(L, 5);
	const bool ctrl  = lua_toboolean(L, 6);
	const bool meta  = lua_toboolean(L, 7);
	const bool shift = lua_toboolean(L, 8);

	const bool success = guihandler->SetActiveCommand(cmdIndex, button, lmb, rmb,
	                                                  alt, ctrl, meta, shift);
	lua_pushboolean(L, success);
	return 1;
}


static int SetActiveCommandByAction(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	if (guihandler == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	const string text = lua_tostring(L, 1);
	const Action action(text);
	CKeySet ks;
	if (args >= 2) {
		const string ksText = lua_tostring(L, 2);
		ks.Parse(ksText);
	}
	const bool success = guihandler->SetActiveCommand(action, ks, 0);
	lua_pushboolean(L, success);
	return 1;
}


int LuaUnsyncedCtrl::SetActiveCommand(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	if (guihandler == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if (args < 1) {
		luaL_error(L, "Incorrect arguments to SetActiveCommand()");
	}
	if (lua_isnumber(L, 1)) {
		return SetActiveCommandByIndex(L);
	}
	if (lua_isstring(L, 1)) {
		return SetActiveCommandByAction(L);
	}
	luaL_error(L, "Incorrect arguments to SetActiveCommand()");
	return 0;
}


int LuaUnsyncedCtrl::ForceLayoutUpdate(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	if (guihandler == NULL) {
		return 0;
	}
	guihandler->ForceLayoutUpdate();
	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::WarpMouse(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to WarpMouse()");
	}
	const int x = lua_toint(L, 1);
	const int y = gu->viewSizeY - lua_toint(L, 2) - 1;
	mouse->WarpMouse(x, y);
	return 0;
}


int LuaUnsyncedCtrl::SetMouseCursor(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	mouse->SetCursor(luaL_checkstring(L, 1));
	if (lua_israwnumber(L, 2)) {
		mouse->cursorScale = lua_tonumber(L, 2);
	}
	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::SetCameraOffset(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	if (camera == NULL) {
		return 0;
	}
	const float px = luaL_optfloat(L, 1, 0.0f);
	const float py = luaL_optfloat(L, 2, 0.0f);
	const float pz = luaL_optfloat(L, 3, 0.0f);
	const float tx = luaL_optfloat(L, 4, 0.0f);
	const float ty = luaL_optfloat(L, 5, 0.0f);
	const float tz = luaL_optfloat(L, 6, 0.0f);
	camera->posOffset = float3(px, py, pz);
	camera->tiltOffset = float3(tx, ty, tz);
	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::SetLosViewColors(lua_State* L)
{
	float red[4];
	float green[4];
	float blue[4];
	if ((ParseFloatArray(L, 1, red,   4) != 4) ||
	    (ParseFloatArray(L, 2, green, 4) != 4) ||
	    (ParseFloatArray(L, 3, blue,  4) != 4)) {
		luaL_error(L, "Incorrect arguments to SetLosViewColors()");
	}
	const int scale = CBaseGroundDrawer::losColorScale;
	CBaseGroundDrawer *gd = readmap->GetGroundDrawer();
	gd->alwaysColor[0] = (int)(scale *   red[0]);
	gd->alwaysColor[1] = (int)(scale * green[0]);
	gd->alwaysColor[2] = (int)(scale *  blue[0]);
	gd->losColor[0]    = (int)(scale *   red[1]);
	gd->losColor[1]    = (int)(scale * green[1]);
	gd->losColor[2]    = (int)(scale *  blue[1]);
	gd->radarColor[0]  = (int)(scale *   red[2]);
	gd->radarColor[1]  = (int)(scale * green[2]);
	gd->radarColor[2]  = (int)(scale *  blue[2]);
	gd->jamColor[0]    = (int)(scale *   red[3]);
	gd->jamColor[1]    = (int)(scale * green[3]);
	gd->jamColor[2]    = (int)(scale *  blue[3]);
	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::GetConfigInt(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	const string name = luaL_checkstring(L, 1);
	const int def     = luaL_optint(L, 2, 0);
	const int value = configHandler->Get(name, def);
	lua_pushnumber(L, value);
	return 1;
}


int LuaUnsyncedCtrl::SetConfigInt(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	const string name = luaL_checkstring(L, 1);
	const int value   = luaL_checkint(L, 2);
	configHandler->Set(name, value);
	return 0;
}


int LuaUnsyncedCtrl::GetConfigString(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	const string name = luaL_checkstring(L, 1);
	const string def  = luaL_optstring(L, 2, "");
	const string value = configHandler->GetString(name, def);
	lua_pushstring(L, value.c_str());
	return 1;
}


int LuaUnsyncedCtrl::SetConfigString(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	const string name  = luaL_checkstring(L, 1);
	const string value = luaL_checkstring(L, 2);
	configHandler->SetString(name, value);
	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::CreateDir(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	const string dir = luaL_checkstring(L, 1);

	// keep directories within the Spring directory
	if ((dir[0] == '/') || (dir[0] == '\\') ||
	    (strstr(dir.c_str(), "..") != NULL) ||
	    ((dir.size() > 0) && (dir[1] == ':'))) {
		luaL_error(L, "Invalid CreateDir() access: %s", dir.c_str());
	}
	const bool success = filesystem.CreateDirectory(dir);
	lua_pushboolean(L, success);
	return 1;
}


/******************************************************************************/

int LuaUnsyncedCtrl::MakeFont(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}

	int tableIndex = 1;
	string inputFile;
	string inputData;
	if (lua_israwstring(L, 1)) {
		inputFile = lua_tostring(L, 1);
		tableIndex++;
	}

	FontTexture::Reset();
	if (lua_istable(L, tableIndex)) {
		for (lua_pushnil(L); lua_next(L, tableIndex) != 0; lua_pop(L, 1)) {
			if (lua_israwstring(L, -2)) {
				const string key = lua_tostring(L, -2);
				if (lua_israwstring(L, -1)) {
					if (key == "outName") {
						FontTexture::SetOutBaseName(lua_tostring(L, -1));
					}
					else if (key == "inData") {
						size_t len = 0;
						const char* ptr = lua_tolstring(L, -1, &len);
						inputData.assign(ptr, len);
						FontTexture::SetInData(inputData);
					}
				}
				else if (lua_isnumber(L, -1)) {
					const unsigned int value = (unsigned int)lua_tonumber(L, -1);
					if (key == "height") {
						FontTexture::SetFontHeight(value);
					} else if (key == "texWidth") {
						FontTexture::SetTextureWidth(value);
					} else if (key == "minChar") {
						FontTexture::SetMinChar(value);
					} else if (key == "maxChar") {
						FontTexture::SetMaxChar(value);
					} else if (key == "outlineMode") {
						FontTexture::SetOutlineMode(value);
					} else if (key == "outlineRadius") {
						FontTexture::SetOutlineRadius(value);
					} else if (key == "outlineWeight") {
						FontTexture::SetOutlineWeight(value);
					} else if (key == "padding") {
						FontTexture::SetPadding(value);
					} else if (key == "stuffing") {
						FontTexture::SetStuffing(value);
					} else if (key == "debug") {
						FontTexture::SetDebugLevel(value);
					}
				}
			}
		}
	}

	if (!inputFile.empty()) {
		// inputData has the override
		FontTexture::SetInFileName(inputFile);
	}

	lua_pushboolean(L, FontTexture::Execute());
	return 1;
}


/******************************************************************************/

int LuaUnsyncedCtrl::SetUnitDefIcon(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
		luaL_error(L,
			"Incorrect arguments to SetUnitDefIcon(unitDefID, \"icon\")");
	}
	const int unitDefID = lua_toint(L, 1);
	const UnitDef* ud = unitDefHandler->GetUnitByID(unitDefID);
	if (ud == NULL) {
		return 0;
	}

	ud->iconType = iconHandler->GetIcon(lua_tostring(L, 2));

	// set decoys to the same icon
	map<int, set<int> >::const_iterator fit;

	if (ud->decoyDef) {
		ud->decoyDef->iconType = ud->iconType;
		fit = unitDefHandler->decoyMap.find(ud->decoyDef->id);
	} else {
		fit = unitDefHandler->decoyMap.find(ud->id);
	}
	if (fit != unitDefHandler->decoyMap.end()) {
		const set<int>& decoySet = fit->second;
		set<int>::const_iterator dit;
		for (dit = decoySet.begin(); dit != decoySet.end(); ++dit) {
  		const UnitDef* decoyDef = unitDefHandler->GetUnitByID(*dit);
			decoyDef->iconType = ud->iconType;
		}
	}

	return 0;
}


int LuaUnsyncedCtrl::SetUnitDefImage(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}

	const int unitDefID = luaL_checkint(L, 1);
	const UnitDef* ud = unitDefHandler->GetUnitByID(unitDefID);
	if (ud == NULL) {
		return 0;
	}

	if (lua_isnoneornil(L, 2)) {
		// reset to default texture
		unitDefHandler->SetUnitDefImage(ud, ud->buildPicName);
		return 0;
	}

	if (!lua_israwstring(L, 2)) {
		return 0;
	}
	const string texName = lua_tostring(L, 2);

	if (texName[0] == LuaTextures::prefix) { // '!'
		LuaTextures& textures = CLuaHandle::GetActiveTextures();
		const LuaTextures::Texture* tex = textures.GetInfo(texName);
		if (tex == NULL) {
			return 0;
		}
		unitDefHandler->SetUnitDefImage(ud, tex->id, tex->xsize, tex->ysize);
	} else {
		unitDefHandler->SetUnitDefImage(ud, texName);
	}
	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::SetUnitGroup(lua_State* L)
{
	GML_RECMUTEX_LOCK(group); // SetUnitGroup

	if (!CheckModUICtrl()) {
		return 0;
	}
	if (gs->noHelperAIs) {
		return 0;
	}

	CUnit* unit = ParseRawUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L);
	if ((args < 2) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetUnitGroup()");
	}
	const int groupID = lua_toint(L, 2);

	if (groupID == -1) {
		unit->SetGroup(NULL);
		return 0;
	}

	const vector<CGroup*>& groups = grouphandlers[gu->myTeam]->groups;
	if ((groupID < 0) || (groupID >= (int)groups.size())) {
		return 0;
	}

	CGroup* group = groups[groupID];
	if (group != NULL) {
		unit->SetGroup(group);
	}
	return 0;
}


/******************************************************************************/

static void ParseUnitMap(lua_State* L, const char* caller,
                         int table, vector<int>& unitIDs)
{
	if (!lua_istable(L, table)) {
		luaL_error(L, "%s(): error parsing unit map", caller);
	}
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_israwnumber(L, -2)) {
			CUnit* unit = ParseCtrlUnit(L, __FUNCTION__, -2); // the key
			if (unit != NULL && !unit->noSelect) {
				unitIDs.push_back(unit->id);
			}
		}
	}
}


static void ParseUnitArray(lua_State* L, const char* caller,
                           int table, vector<int>& unitIDs)
{
	if (!lua_istable(L, table)) {
		luaL_error(L, "%s(): error parsing unit array", caller);
	}
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_israwnumber(L, -2) && lua_isnumber(L, -1)) {   // avoid 'n'
			CUnit* unit = ParseCtrlUnit(L, __FUNCTION__, -1); // the value
			if (unit != NULL && !unit->noSelect) {
				unitIDs.push_back(unit->id);
			}
		}
	}
	return;
}


/******************************************************************************/

static bool CanGiveOrders()
{
	if (gs->frameNum <= 0) {
		return false;
	}
	if (gs->noHelperAIs) {
		return false;
	}
	if (gs->godMode) {
		return true;
	}
	if (gu->spectating) {
		return false;
	}
	const CLuaHandle* lh = CLuaHandle::GetActiveHandle();
	const int ctrlTeam = lh->GetCtrlTeam();
	// FIXME ? (correct? warning / error?)
	if ((ctrlTeam != gu->myTeam) || (ctrlTeam < 0)) {
		return false;
	}
	return true;
}


int LuaUnsyncedCtrl::GiveOrder(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	if (!CanGiveOrders()) {
		return 1;
	}

	Command cmd;
	LuaUtils::ParseCommand(L, __FUNCTION__, 1, cmd);

	selectedUnits.GiveCommand(cmd);

	lua_pushboolean(L, true);

	return 1;
}


int LuaUnsyncedCtrl::GiveOrderToUnit(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	if (!CanGiveOrders()) {
		lua_pushboolean(L, false);
		return 1;
	}

	CUnit* unit = ParseCtrlUnit(L, __FUNCTION__, 1);
	if (unit == NULL || unit->noSelect) {
		lua_pushboolean(L, false);
		return 1;
	}

	Command cmd;
	LuaUtils::ParseCommand(L, __FUNCTION__, 2, cmd);

	net->Send(CBaseNetProtocol::Get().SendAICommand(gu->myPlayerNum, unit->id, cmd.id, cmd.options, cmd.params));

	lua_pushboolean(L, true);
	return 1;
}


int LuaUnsyncedCtrl::GiveOrderToUnitMap(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	if (!CanGiveOrders()) {
		lua_pushboolean(L, false);
		return 1;
	}

	// unitIDs
	vector<int> unitIDs;
	ParseUnitMap(L, __FUNCTION__, 1, unitIDs);
	const int count = (int)unitIDs.size();

	if (count <= 0) {
		lua_pushboolean(L, false);
		return 1;
	}

	Command cmd;
	LuaUtils::ParseCommand(L, __FUNCTION__, 2, cmd);

	vector<Command> commands;
	commands.push_back(cmd);
	selectedUnits.SendCommandsToUnits(unitIDs, commands);

	lua_pushboolean(L, true);
	return 1;
}


int LuaUnsyncedCtrl::GiveOrderToUnitArray(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	if (!CanGiveOrders()) {
		lua_pushboolean(L, false);
		return 1;
	}

	// unitIDs
	vector<int> unitIDs;
	ParseUnitArray(L, __FUNCTION__, 1, unitIDs);
	const int count = (int)unitIDs.size();

	if (count <= 0) {
		lua_pushboolean(L, false);
		return 1;
	}

	Command cmd;
	LuaUtils::ParseCommand(L, __FUNCTION__, 2, cmd);

	vector<Command> commands;
	commands.push_back(cmd);
	selectedUnits.SendCommandsToUnits(unitIDs, commands);

	lua_pushboolean(L, true);
	return 1;
}


int LuaUnsyncedCtrl::GiveOrderArrayToUnitMap(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	if (!CanGiveOrders()) {
		lua_pushboolean(L, false);
		return 1;
	}

	// unitIDs
	vector<int> unitIDs;
	ParseUnitMap(L, __FUNCTION__, 1, unitIDs);

	// commands
	vector<Command> commands;
	LuaUtils::ParseCommandArray(L, __FUNCTION__, 2, commands);

	if ((unitIDs.size() <= 0) || (commands.size() <= 0)) {
		lua_pushboolean(L, false);
		return 1;
	}

	selectedUnits.SendCommandsToUnits(unitIDs, commands);

	lua_pushboolean(L, true);
	return 1;
}


int LuaUnsyncedCtrl::GiveOrderArrayToUnitArray(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	if (!CanGiveOrders()) {
		lua_pushboolean(L, false);
		return 1;
	}

	// unitIDs
	vector<int> unitIDs;
	ParseUnitArray(L, __FUNCTION__, 1, unitIDs);

	// commands
	vector<Command> commands;
	LuaUtils::ParseCommandArray(L, __FUNCTION__, 2, commands);

	if ((unitIDs.size() <= 0) || (commands.size() <= 0)) {
		lua_pushboolean(L, false);
		return 1;
	}

	selectedUnits.SendCommandsToUnits(unitIDs, commands);

	lua_pushboolean(L, true);
	return 1;
}


/******************************************************************************/

static string GetRawMsg(lua_State* L, const char* caller, int index)
{
	if (!lua_israwstring(L, index)) {
		luaL_error(L, "Incorrect arguments to %s", caller);
	}
	size_t len;
	const char* str = lua_tolstring(L, index, &len);
	const string tmpMsg(str, len);
	return tmpMsg;
}


int LuaUnsyncedCtrl::SendLuaUIMsg(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	const string msg = GetRawMsg(L, __FUNCTION__, 1);
	std::vector<boost::uint8_t> data(msg.size());
	std::copy(msg.begin(), msg.end(), data.begin());
	const string mode = luaL_optstring(L, 2, "");
	unsigned char modeNum = 0;
	if ((mode == "s") || (mode == "specs")) {
		modeNum = 's';
	}
	else if ((mode == "a") || (mode == "allies")) {
		modeNum = 'a';
	}
	else if (!mode.empty()) {
		luaL_error(L, "Unknown SendLuaUIMsg() mode");
	}
	net->Send(CBaseNetProtocol::Get().SendLuaMsg(gu->myPlayerNum, LUA_HANDLE_ORDER_UI, modeNum, data));
	return 0;
}


int LuaUnsyncedCtrl::SendLuaGaiaMsg(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	const string msg = GetRawMsg(L, __FUNCTION__, 1);
	std::vector<boost::uint8_t> data(msg.size());
	std::copy(msg.begin(), msg.end(), data.begin());
	net->Send(CBaseNetProtocol::Get().SendLuaMsg(gu->myPlayerNum, LUA_HANDLE_ORDER_GAIA, 0, data));
	return 0;
}


int LuaUnsyncedCtrl::SendLuaRulesMsg(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	const string msg = GetRawMsg(L, __FUNCTION__, 1);
	std::vector<boost::uint8_t> data(msg.size());
	std::copy(msg.begin(), msg.end(), data.begin());
	net->Send(CBaseNetProtocol::Get().SendLuaMsg(gu->myPlayerNum, LUA_HANDLE_ORDER_RULES, 0, data));
	return 0;
}


/******************************************************************************/

int LuaUnsyncedCtrl::SetShareLevel(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	if (gu->spectating || gs->noHelperAIs || (gs->frameNum <= 0)) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isstring(L, 1) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetShareLevel(\"type\", level");
	}

	const string shareType = lua_tostring(L, 1);
	const float shareLevel = max(0.0f, min(1.0f, lua_tofloat(L, 2)));

	if (shareType == "metal") {
		net->Send(CBaseNetProtocol::Get().SendSetShare(gu->myPlayerNum, gu->myTeam, shareLevel, teamHandler->Team(gu->myTeam)->energyShare));
	}
	else if (shareType == "energy") {
		net->Send(CBaseNetProtocol::Get().SendSetShare(gu->myPlayerNum, gu->myTeam,	teamHandler->Team(gu->myTeam)->metalShare, shareLevel));
	}
	else {
		logOutput.Print("SetShareLevel() unknown resource: %s", shareType.c_str());
	}
	return 0;
}


int LuaUnsyncedCtrl::ShareResources(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	if (gu->spectating || gs->noHelperAIs || (gs->frameNum <= 0)) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isstring(L, 2) ||
	    ((args >= 3) && !lua_isnumber(L, 3))) {
		luaL_error(L, "Incorrect arguments to ShareResources()");
	}
	const int teamID = lua_toint(L, 1);
	if ((teamID < 0) || (teamID >= teamHandler->ActiveTeams())) {
		return 0;
	}
	const CTeam* team = teamHandler->Team(teamID);
	if ((team == NULL) || team->isDead) {
		return 0;
	}
	const string& type = lua_tostring(L, 2);
	if (type == "units") {
		// update the selection, and clear the unit command queues
		Command c;
		c.id = CMD_STOP;
		selectedUnits.GiveCommand(c, false);
		net->Send(CBaseNetProtocol::Get().SendShare(gu->myPlayerNum, teamID, 1, 0.0f, 0.0f));
		selectedUnits.ClearSelected();
	}
	else if (args >= 3) {
		const float amount = lua_tofloat(L, 3);
		if (type == "metal") {
			net->Send(CBaseNetProtocol::Get().SendShare(gu->myPlayerNum, teamID, 0, amount, 0.0f));
		}
		else if (type == "energy") {
			net->Send(CBaseNetProtocol::Get().SendShare(gu->myPlayerNum, teamID, 0, 0.0f, amount));
		}
	}
	return 0;
}


/******************************************************************************/
/******************************************************************************/


int LuaUnsyncedCtrl::SetLastMessagePosition(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 3) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
		luaL_error(L, "Incorrect arguments to SetLastMessagePosition(x, y, z)");
	}
	const float3 pos(lua_tofloat(L, 1),
	                 lua_tofloat(L, 2),
	                 lua_tofloat(L, 3));

	logOutput.SetLastMsgPos(pos);

	return 0;
}

/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::MarkerAddPoint(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	if (inMapDrawer == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 3) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2)  || !lua_isnumber(L, 3) ||
	    ((args >= 4) && !lua_isstring(L, 4))) {
		luaL_error(L, "Incorrect arguments to MarkerAddPoint(x, y, z[, text])");
	}
	const float3 pos(lua_tofloat(L, 1),
	                 lua_tofloat(L, 2),
	                 lua_tofloat(L, 3));
	string text = "";
	if (args >= 4) {
	  text = lua_tostring(L, 4);
	}

	inMapDrawer->SendPoint(pos, text);

	return 0;
}


int LuaUnsyncedCtrl::MarkerAddLine(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	if (inMapDrawer == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args != 6) ||
	    !lua_isstring(L, 1) || !lua_isnumber(L, 2) ||
	    !lua_isnumber(L, 3) || !lua_isnumber(L, 4) ||
	    !lua_isnumber(L, 5) || !lua_isnumber(L, 6)) {
		luaL_error(L,
			"Incorrect arguments to MarkerAddLine(x1, y1, z1, x2, y2, z2)");
	}
	const float3 pos1(lua_tofloat(L, 1),
	                  lua_tofloat(L, 2),
	                  lua_tofloat(L, 3));
	const float3 pos2(lua_tofloat(L, 4),
	                  lua_tofloat(L, 5),
	                  lua_tofloat(L, 6));

	inMapDrawer->SendLine(pos1, pos2);

	return 0;
}


int LuaUnsyncedCtrl::MarkerErasePosition(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}
	if (inMapDrawer == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args != 3) ||
	    !lua_isstring(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
		luaL_error(L, "Incorrect arguments to MarkerDeletePositionl(x, y, z)");
	}
	const float3 pos(lua_tofloat(L, 1),
	                 lua_tofloat(L, 2),
	                 lua_tofloat(L, 3));

	inMapDrawer->SendErase(pos);

	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::SetDrawSelectionInfo(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if (args != 1 || !lua_isboolean(L, 1)) {
		luaL_error(L, "Incorrect arguments to SetDrawSelectionInfo(bool)");
	}

	if (guihandler)
		guihandler->SetDrawSelectionInfo(lua_toboolean(L, 1));

	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedCtrl::SetBuildSpacing(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if (args != 1 || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to SetBuildSpacing(int)");
	}

	if (guihandler)
		guihandler->SetBuildSpacing(lua_tointeger(L, 1));

	return 0;
}

int LuaUnsyncedCtrl::SetBuildFacing(lua_State* L)
{
	if (!CheckModUICtrl()) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if (args != 1 || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to SetBuildFacing(int)");
	}

	if (guihandler)
		guihandler->SetBuildFacing(lua_tointeger(L, 1));

	return 0;
}
/******************************************************************************/
/******************************************************************************/
