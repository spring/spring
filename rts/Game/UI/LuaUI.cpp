#include "StdAfx.h"
// LuaUI.cpp: implementation of the CLuaUI class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaUI.h"
#include <set>
#include <cctype>
#include <SDL_keysym.h>
#include <SDL_mouse.h>
#include <SDL_timer.h>
using namespace std;

extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

#include "Lua/LuaCallInHandler.h"
#include "Lua/LuaUtils.h"
#include "Lua/LuaConstGL.h"
#include "Lua/LuaConstCMD.h"
#include "Lua/LuaConstCMDTYPE.h"
#include "Lua/LuaConstGame.h"
#include "Lua/LuaSyncedRead.h"
#include "Lua/LuaUnsyncedCall.h"
#include "Lua/LuaUnsyncedCtrl.h"
#include "Lua/LuaUnsyncedRead.h"
#include "Lua/LuaFeatureDefs.h"
#include "Lua/LuaUnitDefs.h"
#include "Lua/LuaWeaponDefs.h"
#include "Lua/LuaOpenGL.h"
#include "Lua/LuaVFS.h"
#include "ExternalAI/GlobalAIHandler.h"
#include "ExternalAI/Group.h"
#include "ExternalAI/GroupHandler.h"
#include "Game/command.h"
#include "Game/Camera.h"
#include "Game/CameraController.h"
#include "Game/Game.h"
#include "Game/GameHelper.h"
#include "Game/PlayerRoster.h"
#include "Game/SelectedUnits.h"
#include "Game/Team.h"
#include "Game/UI/CommandColors.h"
#include "Game/UI/CursorIcons.h"
#include "Game/UI/GuiHandler.h"
#include "Game/UI/InfoConsole.h"
#include "Game/UI/KeyCodes.h"
#include "Game/UI/KeySet.h"
#include "Game/UI/KeyBindings.h"
#include "Game/UI/MouseHandler.h"
#include "Rendering/InMapDraw.h"
#include "Rendering/FontTexture.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "System/LogOutput.h"
#include "System/NetProtocol.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/Platform/ConfigHandler.h"
#include "System/Platform/FileSystem.h"

extern Uint8 *keys;
extern GLfloat LightDiffuseLand[];
extern GLfloat LightAmbientLand[];


CLuaUI* luaUI = NULL;


const int CMD_INDEX_OFFSET = 1; // starting index for command descriptions


/******************************************************************************/
/******************************************************************************/


void CLuaUI::LoadHandler()
{
	if (luaUI) {
		return;
	}
	
	SAFE_NEW CLuaUI();
	
	if (luaUI->L == NULL) {
		delete luaUI;
	}
}


void CLuaUI::FreeHandler()
{
	delete luaUI;
}


/******************************************************************************/

Uint32 CLuaUI::lastUpdateTime = SDL_GetTicks();
float  CLuaUI::lastUpdateSeconds = 0.0f;


CLuaUI::CLuaUI()
: CLuaHandle("LuaUI", LUA_HANDLE_ORDER_UI, true, NULL)
{
	luaUI = this;
	
	if (L == NULL) {
		return;
	}

	UpdateTeams();

	lastUpdateTime = SDL_GetTicks();
	lastUpdateSeconds = 0.0f;

	const string code = LoadFile("gui.lua");
	if (code.empty()) {
		KillLua();
		return;
	}

	// load the standard libraries
	luaopen_base(L);
	luaopen_io(L);
	luaopen_math(L);
	luaopen_table(L);
	luaopen_string(L);
	luaopen_debug(L);

	lua_pushvalue(L, LUA_GLOBALSINDEX);

	AddBasicCalls(); // into Global

	// load the spring libraries
	if (!LoadCFunctions(L)                                                 ||
	    !AddEntriesToTable(L, "VFS",         LuaVFS::PushUnsynced)         ||
	    !AddEntriesToTable(L, "UnitDefs",    LuaUnitDefs::PushEntries)     ||
	    !AddEntriesToTable(L, "WeaponDefs",  LuaWeaponDefs::PushEntries)   ||
	    !AddEntriesToTable(L, "FeatureDefs", LuaFeatureDefs::PushEntries)  ||
	    !AddEntriesToTable(L, "Script",      LuaUnsyncedCall::PushEntries) ||
	    !AddEntriesToTable(L, "Spring",      LuaSyncedRead::PushEntries)   ||
	    !AddEntriesToTable(L, "Spring",      LuaUnsyncedCtrl::PushEntries) ||
	    !AddEntriesToTable(L, "Spring",      LuaUnsyncedRead::PushEntries) ||
	    !AddEntriesToTable(L, "gl",          LuaOpenGL::PushEntries)       ||
	    !AddEntriesToTable(L, "GL",          LuaConstGL::PushEntries)      ||
	    !AddEntriesToTable(L, "Game",        LuaConstGame::PushEntries)    ||
	    !AddEntriesToTable(L, "CMD",         LuaConstCMD::PushEntries)     ||
	    !AddEntriesToTable(L, "CMDTYPE",     LuaConstCMDTYPE::PushEntries)) {
		KillLua();
		return;
	}

	lua_settop(L, 0);

	if (!LoadCode(code, "gui.lua")) {
		KillLua();
		return;
	}
	
	// register for call-ins
	luaCallIns.AddHandle(this);
}


CLuaUI::~CLuaUI()
{
	if (L != NULL) {
		Shutdown();
	}
	luaUI = NULL;
}


void CLuaUI::KillLua()
{
	if (L != NULL) {
		lua_close(L);
		L = NULL;
	}
}


string CLuaUI::LoadFile(const string& filename) const
{
	CFileHandler f(filename, CFileHandler::OnlyRawFS);

	string code;
	if (!f.LoadStringData(code)) {
		code.clear();
	}
	return code;
}


bool CLuaUI::HasCallIn(const string& callInName)
{
	if (L == NULL) {
		return false;
	}

	// never allow these calls;
	// use UpdateLayout, DrawWorldItems, and DrawScreenItems instead
	if ((callInName == "Update")    ||
	    (callInName == "DrawWorld") ||
	    (callInName == "DrawScreen")) {
		return false;
	}

	lua_getglobal(L, callInName.c_str());
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	lua_pop(L, 1);
	return true;
}


void CLuaUI::UpdateTeams()
{
	if (luaUI) {
		luaUI->fullCtrl = false;
		luaUI->fullRead = gu->spectatingFullView;
		luaUI->ctrlTeam = gu->spectating ? NoAccessTeam : gu->myTeam;
		luaUI->readTeam = luaUI->fullRead ? AllAccessTeam : gu->myTeam;
		luaUI->readAllyTeam = luaUI->fullRead ? AllAccessTeam : gu->myAllyTeam;
		luaUI->selectTeam = gu->spectatingFullSelect ? AllAccessTeam : gu->myTeam;
	}
}


/******************************************************************************/
/******************************************************************************/

bool CLuaUI::LoadCFunctions(lua_State* L)
{
	lua_newtable(L);

#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(SendCommands);
	REGISTER_LUA_CFUNC(GiveOrder);
	REGISTER_LUA_CFUNC(GiveOrderToUnit);
	REGISTER_LUA_CFUNC(GiveOrderToUnitMap);
	REGISTER_LUA_CFUNC(GiveOrderToUnitArray);
	REGISTER_LUA_CFUNC(GiveOrderArrayToUnitMap);
	REGISTER_LUA_CFUNC(GiveOrderArrayToUnitArray);

	REGISTER_LUA_CFUNC(GetFPS);
	REGISTER_LUA_CFUNC(GetLastUpdateSeconds);

	REGISTER_LUA_CFUNC(SetActiveCommand);
	REGISTER_LUA_CFUNC(GetActiveCommand);
	REGISTER_LUA_CFUNC(GetDefaultCommand);
	REGISTER_LUA_CFUNC(GetActiveCmdDescs);
	REGISTER_LUA_CFUNC(GetActiveCmdDesc);
	REGISTER_LUA_CFUNC(GetCmdDescIndex);

	REGISTER_LUA_CFUNC(GetMouseState);
	REGISTER_LUA_CFUNC(GetMouseCursor);
	REGISTER_LUA_CFUNC(SetMouseCursor);
	REGISTER_LUA_CFUNC(AddMouseCursor);
	REGISTER_LUA_CFUNC(WarpMouse);

	REGISTER_LUA_CFUNC(GetKeyState);
	REGISTER_LUA_CFUNC(GetModKeyState);
	REGISTER_LUA_CFUNC(GetPressedKeys);

	REGISTER_LUA_CFUNC(GetKeyCode);
	REGISTER_LUA_CFUNC(GetKeySymbol);
	REGISTER_LUA_CFUNC(GetKeyBindings);
	REGISTER_LUA_CFUNC(GetActionHotKeys);

	REGISTER_LUA_CFUNC(GetConsoleBuffer);
	REGISTER_LUA_CFUNC(GetCurrentTooltip);

	REGISTER_LUA_CFUNC(GetConfigInt);
	REGISTER_LUA_CFUNC(SetConfigInt);
	REGISTER_LUA_CFUNC(GetConfigString);
	REGISTER_LUA_CFUNC(SetConfigString);

	REGISTER_LUA_CFUNC(CreateDir);
	REGISTER_LUA_CFUNC(MakeFont);
	REGISTER_LUA_CFUNC(SetUnitDefIcon);

	REGISTER_LUA_CFUNC(GetMyAllyTeamID);
	REGISTER_LUA_CFUNC(GetMyTeamID);
	REGISTER_LUA_CFUNC(GetMyPlayerID);

	REGISTER_LUA_CFUNC(GetGroupList);
	REGISTER_LUA_CFUNC(GetSelectedGroup);
	REGISTER_LUA_CFUNC(GetGroupAIName);
	REGISTER_LUA_CFUNC(GetGroupAIList);

	REGISTER_LUA_CFUNC(GetUnitGroup);
	REGISTER_LUA_CFUNC(SetUnitGroup);

	REGISTER_LUA_CFUNC(GetGroupUnits);
	REGISTER_LUA_CFUNC(GetGroupUnitsSorted);
	REGISTER_LUA_CFUNC(GetGroupUnitsCounts);

	REGISTER_LUA_CFUNC(SetShareLevel);
	REGISTER_LUA_CFUNC(ShareResources);

	REGISTER_LUA_CFUNC(MarkerAddPoint);
	REGISTER_LUA_CFUNC(MarkerAddLine);
	REGISTER_LUA_CFUNC(MarkerErasePosition);

	lua_setglobal(L, "Spring");

	return true;
}


/******************************************************************************/
/******************************************************************************/

void CLuaUI::Shutdown()
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("Shutdown");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return;
	}

	// call the routine
	RunCallIn(cmdStr, 0, 0);
	return;
}


bool CLuaUI::ConfigCommand(const string& command)
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("ConfigureLayout");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return true; // the call is not defined
	}

	lua_pushstring(L, command.c_str());

	// call the routine
	return RunCallIn(cmdStr, 1, 0);
}


bool CLuaUI::AddConsoleLines(lua_State* L)
{
	CInfoConsole* ic = game->infoConsole;
	if (ic == NULL) {
		return true;
	}

	vector<CInfoConsole::RawLine> lines;
	ic->GetNewRawLines(lines);

	const int count = (int)lines.size();
	for (int i = 0; i < count; i++) {
		const CInfoConsole::RawLine& rl = lines[i];

		lua_settop(L, 0);
		static const LuaHashString cmdStr("AddConsoleLine");
		if (!cmdStr.GetGlobalFunc(L)) {
			lua_settop(L, 0);
			return true; // the call is not defined
		}

		lua_pushstring(L, rl.text.c_str());
		lua_pushnumber(L, rl.priority);

		// call the function
		if (!RunCallIn(cmdStr, 2, 0)) {
			return false;
		}

		lua_settop(L, 0);
	}
	return true;
}


bool CLuaUI::UpdateLayout(bool commandsChanged, int activePage)
{
	if (killMe) {
		string msg = name;
		if (!killMsg.empty()) {
			msg += ": " + killMsg;
		}
		logOutput.Print("Disabled %s\n", msg.c_str());
		delete this;
		return true; // force the update
	}

	// update the timer
	const Uint32 newTime = SDL_GetTicks();
	lastUpdateSeconds = 0.001f * (newTime - lastUpdateTime);
	lastUpdateTime = newTime;

	// add new lines from the console buffer
	AddConsoleLines(L);

	lua_settop(L, 0);
	static const LuaHashString cmdStr("UpdateLayout");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return false;
	}

	lua_pushboolean(L, commandsChanged);
	lua_pushnumber(L,  activePage);
	lua_pushboolean(L, keys[SDLK_LALT]);
	lua_pushboolean(L, keys[SDLK_LCTRL]);
	lua_pushboolean(L, keys[SDLK_LMETA]);
	lua_pushboolean(L, keys[SDLK_LSHIFT]);

	if (!RunCallIn(cmdStr, 6, 1)) {
		return false;
	}

	if (!lua_isboolean(L, 1)) {
		logOutput.Print("UpdateLayout() expects a boolean return value\n");
		return false;
	}

	const bool forceLayout = !!lua_toboolean(L, 1);
	lua_settop(L, 0);

	return forceLayout;
}


bool CLuaUI::CommandNotify(const Command& cmd)
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("CommandNotify");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return false; // the call is not defined
	}

	// push the command id
	lua_pushnumber(L, cmd.id);

	// push the params list
	lua_newtable(L);
	for (int p = 0; p < (int)cmd.params.size(); p++) {
		lua_pushnumber(L, p + 1);
		lua_pushnumber(L, cmd.params[p]);
		lua_rawset(L, -3);
	}

	// push the options table
	lua_newtable(L);
	HSTR_PUSH_NUMBER(L, "coded", cmd.options);
	HSTR_PUSH_BOOL(L, "alt",   cmd.options & ALT_KEY);
	HSTR_PUSH_BOOL(L, "ctrl",  cmd.options & CONTROL_KEY);
	HSTR_PUSH_BOOL(L, "shift", cmd.options & SHIFT_KEY);
	HSTR_PUSH_BOOL(L, "right", cmd.options & RIGHT_MOUSE_KEY);

	// call the function
	if (!RunCallIn(cmdStr, 3, 1)) {
		return false;
	}

	// get the results
	const int args = lua_gettop(L);
	if ((args != 1) || !lua_isboolean(L, 1)) {
		logOutput.Print("CommandNotify() bad return value (%i)\n", args);
		lua_pop(L, args);
		return false;
	}

	return !!lua_toboolean(L, 1);
}


bool CLuaUI::GroupChanged(int groupID)
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("GroupChanged");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return false; // the call is not defined
	}

	lua_pushnumber(L, groupID);

	// call the routine
	if (!RunCallIn(cmdStr, 1, 0)) {
		return false;
	}

	return true;
}


/******************************************************************************/

bool CLuaUI::DrawWorldItems()
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("DrawWorldItems");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return true; // the call is not defined
	}

	LuaOpenGL::EnableDrawWorld();

	// call the routine
	bool retval = RunCallIn(cmdStr, 0, 0);

	LuaOpenGL::DisableDrawWorld();

	return retval;
}


bool CLuaUI::DrawScreenItems()
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("DrawScreenItems");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return true; // the call is not defined
	}

	LuaOpenGL::EnableDrawScreen();

	lua_pushnumber(L, gu->viewSizeX);
	lua_pushnumber(L, gu->viewSizeY);

	// call the routine
	bool retval = RunCallIn(cmdStr, 2, 0);

	LuaOpenGL::DisableDrawScreen();

	return retval;
}


/******************************************************************************/

bool CLuaUI::KeyPress(unsigned short key, bool isRepeat)
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("KeyPress");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, key);

	lua_newtable(L);
	HSTR_PUSH_BOOL(L, "alt",   keys[SDLK_LALT]);
	HSTR_PUSH_BOOL(L, "ctrl",  keys[SDLK_LCTRL]);
	HSTR_PUSH_BOOL(L, "meta",  keys[SDLK_LMETA]);
	HSTR_PUSH_BOOL(L, "shift", keys[SDLK_LSHIFT]);

	lua_pushboolean(L, isRepeat);

	CKeySet ks(key, false);
	lua_pushstring(L, ks.GetString(true).c_str());

	// call the function
	if (!RunCallIn(cmdStr, 4, 1)) {
		return false;
	}

	const int args = lua_gettop(L);
	if ((args == 1) && lua_isboolean(L, 1)) {
		return !!lua_toboolean(L, 1);
	}

	return false;
}


bool CLuaUI::KeyRelease(unsigned short key)
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("KeyRelease");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, key);

	lua_newtable(L);
	HSTR_PUSH_BOOL(L, "alt",   keys[SDLK_LALT]);
	HSTR_PUSH_BOOL(L, "ctrl",  keys[SDLK_LCTRL]);
	HSTR_PUSH_BOOL(L, "meta",  keys[SDLK_LMETA]);
	HSTR_PUSH_BOOL(L, "shift", keys[SDLK_LSHIFT]);

	CKeySet ks(key, false);
	lua_pushstring(L, ks.GetString(true).c_str());

	// call the function
	if (!RunCallIn(cmdStr, 3, 1)) {
		return false;
	}

	const int args = lua_gettop(L);
	if ((args == 1) && lua_isboolean(L, 1)) {
		return !!lua_toboolean(L, 1);
	}

	return false;
}


bool CLuaUI::MouseMove(int x, int y, int dx, int dy, int button)
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("MouseMove");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, x - gu->viewPosX);
	lua_pushnumber(L, gu->viewSizeY - y - 1);
	lua_pushnumber(L, dx);
	lua_pushnumber(L, -dy);
	lua_pushnumber(L, button);

	// call the function
	if (!RunCallIn(cmdStr, 5, 1)) {
		return false;
	}

	const int args = lua_gettop(L);
	if ((args == 1) && lua_isboolean(L, -1)) {
		return !!lua_toboolean(L, -1);
	}

	return false;
}


bool CLuaUI::MousePress(int x, int y, int button)
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("MousePress");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, x - gu->viewPosX);
	lua_pushnumber(L, gu->viewSizeY - y - 1);
	lua_pushnumber(L, button);

	// call the function
	if (!RunCallIn(cmdStr, 3, 1)) {
		return false;
	}

	const int args = lua_gettop(L);
	if ((args == 1) && lua_isboolean(L, 1)) {
		return !!lua_toboolean(L, 1);
	}

	return false;
}


int CLuaUI::MouseRelease(int x, int y, int button)
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("MouseRelease");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, x - gu->viewPosX);
	lua_pushnumber(L, gu->viewSizeY - y - 1);
	lua_pushnumber(L, button);

	// call the function
	if (!RunCallIn(cmdStr, 3, 1)) {
		return false;
	}

	const int args = lua_gettop(L);
	if ((args == 1) && lua_isnumber(L, 1)) {
		return ((int)lua_tonumber(L, 1) - 1);
	}

	return -1;
}


bool CLuaUI::IsAbove(int x, int y)
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("IsAbove");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return false; // the call is not defined
	}

	lua_pushnumber(L, x - gu->viewPosX);
	lua_pushnumber(L, gu->viewSizeY - y - 1);

	// call the function
	if (!RunCallIn(cmdStr, 2, 1)) {
		return false;
	}

	const int args = lua_gettop(L);
	if ((args == 1) && lua_isboolean(L, 1)) {
		return !!lua_toboolean(L, 1);
	}

	return false;
}


string CLuaUI::GetTooltip(int x, int y)
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("GetTooltip");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return ""; // the call is not defined
	}

	lua_pushnumber(L, x - gu->viewPosX);
	lua_pushnumber(L, gu->viewSizeY - y - 1);

	// call the function
	if (!RunCallIn(cmdStr, 2, 1)) {
		return "";
	}

	const int args = lua_gettop(L);
	if ((args == 1) && lua_isstring(L, 1)) {
		return string(lua_tostring(L, 1));
	}

	return "";
}


bool CLuaUI::HasLayoutButtons()
{
	lua_settop(L, 0);
	static const LuaHashString cmdStr("LayoutButtons");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return false; // the call is not defined
	}
	lua_settop(L, 0);
	return true;
}


bool CLuaUI::LayoutButtons(int& xButtons, int& yButtons,
                           const vector<CommandDescription>& cmds,
                           vector<int>& removeCmds,
                           vector<CommandDescription>& customCmds,
                           vector<int>& onlyTextureCmds,
                           vector<ReStringPair>& reTextureCmds,
                           vector<ReStringPair>& reNamedCmds,
                           vector<ReStringPair>& reTooltipCmds,
                           vector<ReParamsPair>& reParamsCmds,
                           map<int, int>& buttonList,
                           string& menuName)
{
	customCmds.clear();
	removeCmds.clear();
	reTextureCmds.clear();
	reNamedCmds.clear();
	reTooltipCmds.clear();
	onlyTextureCmds.clear();
	buttonList.clear();
	menuName = "";

	lua_settop(L, 0);
	static const LuaHashString cmdStr("LayoutButtons");
	if (!cmdStr.GetGlobalFunc(L)) {
		lua_settop(L, 0);
		return false;
	}

	lua_pushnumber(L, xButtons);
	lua_pushnumber(L, yButtons);
	lua_pushnumber(L, cmds.size());

	if (!BuildCmdDescTable(L, cmds)) {
		lua_settop(L, 0);
		return false;
	}

	// call the function
	if (!RunCallIn(cmdStr, 4, LUA_MULTRET)) {
		return false;
	}

	// get the results
	const int args = lua_gettop(L);

	if ((args == 1) && (lua_isstring(L, -1)) &&
	    (string(lua_tostring(L, -1)) == "disabled")) {
		return false; // no warnings
	}
	else if (args != 11) {
		logOutput.Print("LayoutButtons() bad number of return values (%i)\n", args);
		lua_pop(L, args);
		return false;
	}

	if (!lua_isstring(L, 1)) {
		logOutput.Print("LayoutButtons() bad xButtons or yButtons values\n");
		lua_pop(L, args);
		return false;
	}
	menuName = lua_tostring(L, 1);

	if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
		logOutput.Print("LayoutButtons() bad xButtons or yButtons values\n");
		lua_pop(L, args);
		return false;
	}
	xButtons = (int)lua_tonumber(L, 2);
	yButtons = (int)lua_tonumber(L, 3);

	if (!GetLuaIntList(L, 4, removeCmds)) {
		logOutput.Print("LayoutButtons() bad removeCommands table\n");
		lua_settop(L, 0);
		return false;
	}

	if (!GetLuaCmdDescList(L, 5, customCmds)) {
		logOutput.Print("LayoutButtons() bad customCommands table\n");
		lua_settop(L, 0);
		return false;
	}

	if (!GetLuaIntList(L, 6, onlyTextureCmds)) {
		logOutput.Print("LayoutButtons() bad onlyTexture table\n");
		lua_settop(L, 0);
		return false;
	}

	if (!GetLuaReStringList(L, 7, reTextureCmds)) {
		logOutput.Print("LayoutButtons() bad reTexture table\n");
		lua_settop(L, 0);
		return false;
	}

	if (!GetLuaReStringList(L, 8, reNamedCmds)) {
		logOutput.Print("LayoutButtons() bad reNamed table\n");
		lua_settop(L, 0);
		return false;
	}

	if (!GetLuaReStringList(L, 9, reTooltipCmds)) {
		logOutput.Print("LayoutButtons() bad reTooltip table\n");
		lua_settop(L, 0);
		return false;
	}

	if (!GetLuaReParamsList(L, 10, reParamsCmds)) {
		logOutput.Print("LayoutButtons() bad reParams table\n");
		lua_settop(L, 0);
		return false;
	}

	if (!GetLuaIntMap(L, 11, buttonList)) {
		logOutput.Print("LayoutButtons() bad buttonList table\n");
		lua_settop(L, 0);
		return false;
	}

	return true;
}


bool CLuaUI::BuildCmdDescTable(lua_State* L,
                               const vector<CommandDescription>& cmds)
{
	lua_newtable(L);

	const int cmdDescCount = (int)cmds.size();
	for (int i = 0; i < cmdDescCount; i++) {
		lua_pushnumber(L, i + CMD_INDEX_OFFSET);
		lua_newtable(L); {
			HSTR_PUSH_NUMBER(L, "id",       cmds[i].id);
			HSTR_PUSH_NUMBER(L, "type",     cmds[i].type);
			HSTR_PUSH_STRING(L, "action",   cmds[i].action.c_str());
			HSTR_PUSH_BOOL(L,   "hidden",   cmds[i].onlyKey);
			HSTR_PUSH_BOOL(L,   "disabled", cmds[i].disabled);

			HSTR_PUSH(L, "params");
			lua_newtable(L);
			for (int p = 0; p < cmds[i].params.size(); p++) {
				lua_pushnumber(L, p + 1);
				lua_pushstring(L, cmds[i].params[p].c_str());
				lua_rawset(L, -3);
			}
			lua_rawset(L, -3);
		}
		lua_rawset(L, -3);
	}
	HSTR_PUSH_NUMBER(L, "n", cmdDescCount);
	return true;
}


bool CLuaUI::GetLuaIntMap(lua_State* L, int index, map<int, int>& intMap)
{
	const int table = index;
	if (!lua_istable(L, table)) {
		return false;
	}
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_isnumber(L, -1) || !lua_isnumber(L, -2)) {
			lua_pop(L, 2);
			return false;
		}
		const int key = (int)lua_tonumber(L, -2);
		const int value = (int)lua_tonumber(L, -1) - CMD_INDEX_OFFSET;
		intMap[key] = value;
	}

	return true;
}


bool CLuaUI::GetLuaIntList(lua_State* L, int index, vector<int>& intList)
{
	const int table = index;
	if (!lua_istable(L, table)) {
		return false;
	}
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_isnumber(L, -1)) { // the value  (key is at -2)
			lua_pop(L, 2);
			return false;
		}
		const int value = (int)lua_tonumber(L, -1) - CMD_INDEX_OFFSET;
		intList.push_back(value);
	}

	return true;
}


bool CLuaUI::GetLuaReStringList(lua_State* L, int index,
                                vector<ReStringPair>& reStringList)
{
	const int table = index;
	if (!lua_istable(L, table)) {
		return false;
	}

	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_isnumber(L, -2) || !lua_isstring(L, -1)) {
			lua_pop(L, 2);
			return false;
		}
		ReStringPair rp;
		rp.cmdIndex = (int)lua_tonumber(L, -2) - CMD_INDEX_OFFSET;
		rp.texture = lua_tostring(L, -1);
		reStringList.push_back(rp);
	}

	return true;
}


bool CLuaUI::GetLuaReParamsList(lua_State* L, int index,
                                vector<ReParamsPair>& reParamsCmds)
{
	const int table = index;
	if (!lua_istable(L, table)) {
		return false;
	}

	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_isnumber(L, -2) || !lua_istable(L, -1)) {
			lua_pop(L, 2);
			return false;
		}
		ReParamsPair paramsPair;
		paramsPair.cmdIndex = (int)lua_tonumber(L, -2) - CMD_INDEX_OFFSET;
		const int paramTable = lua_gettop(L);
		for (lua_pushnil(L); lua_next(L, paramTable) != 0; lua_pop(L, 1)) {
			if (!lua_isnumber(L, -2) || !lua_isstring(L, -1)) {
				lua_pop(L, 4);
				return false;
			}
			const int paramIndex = (int)lua_tonumber(L, -2);
			const string paramValue = lua_tostring(L, -1);
			paramsPair.params[paramIndex] = paramValue;
		}
		reParamsCmds.push_back(paramsPair);
	}

	return true;
}


bool CLuaUI::GetLuaCmdDescList(lua_State* L, int index,
                               vector<CommandDescription>& cmdDescs)
{
	const int table = index;
	if (!lua_istable(L, table)) {
		return false;
	}
	int paramIndex = 1;
	for (lua_rawgeti(L, table, paramIndex);
	     lua_istable(L, -1);
	     lua_pop(L, 1), paramIndex++, lua_rawgeti(L, table, paramIndex)) {
		CommandDescription cd;
		cd.id   = CMD_INTERNAL;
		cd.type = CMDTYPE_CUSTOM;
		const int cmdDescTable = lua_gettop(L);
		for (lua_pushnil(L); lua_next(L, cmdDescTable) != 0; lua_pop(L, 1)) {
			if (lua_isstring(L, -2) && lua_isstring(L, -1)) {
				const string key = StringToLower(lua_tostring(L, -2));
				const string value = lua_tostring(L, -1);
				if (key == "id") {
					cd.id = atoi(value.c_str());
				} else if (key == "type") {
					cd.type = atoi(value.c_str());
				} else if (key == "name") {
					cd.name = value;
				} else if (key == "action") {
					cd.action = value;
				} else if (key == "iconname") {
					cd.iconname = value;
				} else if (key == "mouseicon") {
					cd.mouseicon = value;
				} else if (key == "tooltip") {
					cd.tooltip = value;
				} else if (key == "nwtext") {
					cd.params.push_back(string("$nw$") + value);
				} else if (key == "swtext") {
					cd.params.push_back(string("$sw$") + value);
				} else if (key == "netext") {
					cd.params.push_back(string("$ne$") + value);
				} else if (key == "setext") {
					cd.params.push_back(string("$se$") + value);
				} else {
					logOutput.Print("GetLuaCmdDescList() unknown key  (%s)\n", key.c_str());
				}
			}
			else if (lua_isstring(L, -2) && lua_istable(L, -1)) {
				const string key = StringToLower(lua_tostring(L, -2));
				if (key != "actions") {
					logOutput.Print("GetLuaCmdDescList() non \"actions\" table\n");
					lua_pop(L, 1); // pop the value
					continue;
				}
				const int actionsTable = lua_gettop(L);
				lua_pushnil(L);
				while (lua_next(L, actionsTable) != 0) {
					if (lua_isstring(L, -1)) {
						string action = lua_tostring(L, -1);
						if (action[0] != '@') {
							action = "@@" + action;
						}
						cd.params.push_back(action);
					}
					lua_pop(L, 1); // pop the value
				}
			}
			else {
				logOutput.Print("GetLuaCmdDescList() bad entry\n");
			}
		}
		cmdDescs.push_back(cd);
	}

	if (!lua_isnil(L, -1)) {
		logOutput.Print("GetLuaCmdDescList() entry %i is not a table\n", paramIndex);
		lua_pop(L, 1);
		return false;
	}
	lua_pop(L, 1);

	return true;
}


/******************************************************************************/
/******************************************************************************/
//
// Custom Call-in
//

bool CLuaUI::HasUnsyncedXCall(const string& funcName)
{
	lua_getglobal(L, funcName.c_str());
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	lua_pop(L, 1);
	return true;
}


int CLuaUI::UnsyncedXCall(lua_State* srcState, const string& funcName)
{
	const bool diffStates = (srcState != L);
	const int argCount = lua_gettop(srcState);

	const LuaHashString cmdStr(funcName);
	if (!cmdStr.GetGlobalFunc(L)) {
		return 0;
	}
	lua_insert(L, 1); // move the function to the beginning

	if (diffStates) {
		LuaUtils::CopyData(L, srcState, argCount);
	}

	// call the function
	if (!RunCallIn(cmdStr, argCount, 1)) {
		return 0;
	}

	const int retCount = lua_gettop(L);
	if ((retCount > 0) && diffStates) {
		lua_settop(srcState, 0);
		LuaUtils::CopyData(srcState, L, retCount);
	}

	return retCount;
}


/******************************************************************************/
/******************************************************************************/

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
		luaL_error(L, "Bad unitID in %s() %i\n", caller, unitID);
	}
	CUnit* unit = uh->units[unitID];
	if (unit == NULL) {
		luaL_error(L, "Bad unitID pointer in %s() %i\n", caller, unitID);
	}
	return unit;
}


static inline CUnit* ParseReadUnit(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseRawUnit(L, caller, index);
	if (unit == NULL) {
		return NULL;
	}
	static const int&  readAllyTeam = CLuaHandle::GetActiveReadAllyTeam();
	if (readAllyTeam < 0) {
		return (readAllyTeam == CLuaHandle::AllAccessTeam) ? unit : NULL;
	}
	if ((unit->losStatus[readAllyTeam] & (LOS_INLOS | LOS_INRADAR)) == 0) {
		return NULL;
	}
	return unit;
}


static inline CUnit* ParseCtrlUnit(lua_State* L, const char* caller, int index)
{
	CUnit* unit = ParseRawUnit(L, caller, index);
	if (unit == NULL) {
		return NULL;
	}
	const int ctrlTeam = CLuaHandle::GetActiveHandle()->GetCtrlTeam();
	if (ctrlTeam < 0) {
		return (ctrlTeam == CLuaHandle::AllAccessTeam) ? unit : NULL;
	}
	if (ctrlTeam != unit->team) {
		return NULL;
	}
	return unit;
}


static inline void CheckNoArgs(lua_State* L, const char* funcName)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		luaL_error(L, "%s() takes no arguments", funcName);
	}
}


/******************************************************************************/
/******************************************************************************/
//
// Lua Callbacks
//

int CLuaUI::CreateDir(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to CreateDir()");
	}

	// keep directories within the Spring directory
	const string dir = lua_tostring(L, 1);
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

int CLuaUI::GetFPS(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	if (game) {
		lua_pushnumber(L, game->fps);
	} else {
		lua_pushnumber(L, 0);
	}
	return 1;
}


int CLuaUI::GetLastUpdateSeconds(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, lastUpdateSeconds);
	return 1;
}


int CLuaUI::SendCommands(lua_State* L)
{
	if ((guihandler == NULL) || gs->noHelperAIs) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_istable(L, -1)) {
		luaL_error(L, "Incorrect arguments to SendCommands()");
	}
	vector<string> cmds;
	const int table = lua_gettop(L);
	lua_pushnil(L);
	while (lua_next(L, table) != 0) {
		if (lua_isstring(L, -1)) {
			string action = lua_tostring(L, -1);
			if (action[0] != '@') {
				action = "@@" + action;
			}
			cmds.push_back(action);
		}
		lua_pop(L, 1);
	}
	guihandler->RunCustomCommands(cmds, false);
	return 0;
}


/******************************************************************************/

static int SetActiveCommandByIndex(lua_State* L)
{
	if (guihandler == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	const int cmdIndex = (int)lua_tonumber(L, 1) - CMD_INDEX_OFFSET;
	int button = 1; // LMB
	if ((args >= 2) && lua_isnumber(L, 2)) {
		button = (int)lua_tonumber(L, 2);
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
	if (guihandler == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	const string text = lua_tostring(L, 1);
	const CKeyBindings::Action action(text);
	CKeySet ks;
	if (args >= 2) {
		const string ksText = lua_tostring(L, 2);
		ks.Parse(ksText);
	}
	const bool success = guihandler->SetActiveCommand(action, ks, 0);
	lua_pushboolean(L, success);
	return 1;
}


int CLuaUI::SetActiveCommand(lua_State* L)
{
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


int CLuaUI::GetActiveCommand(lua_State* L)
{
	if (guihandler == NULL) {
		return 0;
	}
	CheckNoArgs(L, __FUNCTION__);

	const vector<CommandDescription>& cmdDescs = guihandler->commands;
	const int cmdDescCount = (int)cmdDescs.size();

	const int inCommand = guihandler->inCommand;
	lua_pushnumber(L, inCommand + CMD_INDEX_OFFSET);
	if ((inCommand < 0) || (inCommand >= cmdDescCount)) {
		return 1;
	}
	lua_pushnumber(L, cmdDescs[inCommand].id);
	lua_pushnumber(L, cmdDescs[inCommand].type);
	lua_pushstring(L, cmdDescs[inCommand].name.c_str());
	return 4;
}


int CLuaUI::GetDefaultCommand(lua_State* L)
{
	if (guihandler == NULL) {
		return 0;
	}
	CheckNoArgs(L, __FUNCTION__);

	const vector<CommandDescription>& cmdDescs = guihandler->commands;
	const int cmdDescCount = (int)cmdDescs.size();

	const int defCmd = guihandler->GetDefaultCommand(mouse->lastx, mouse->lasty);
	lua_pushnumber(L, defCmd + CMD_INDEX_OFFSET);
	if ((defCmd < 0) || (defCmd >= cmdDescCount)) {
		return 1;
	}
	lua_pushnumber(L, cmdDescs[defCmd].id);
	lua_pushnumber(L, cmdDescs[defCmd].type);
	lua_pushstring(L, cmdDescs[defCmd].name.c_str());
	return 4;
}


// FIXME: duplicated in LuaSyncedRead.cpp
static void PushCommandDesc(lua_State* L, const CommandDescription& cd)
{
	lua_newtable(L);

	HSTR_PUSH_NUMBER(L, "id",          cd.id);
	HSTR_PUSH_NUMBER(L, "type",        cd.type);
	HSTR_PUSH_STRING(L, "name",        cd.name);
	HSTR_PUSH_STRING(L, "action",      cd.action);
	HSTR_PUSH_STRING(L, "tooltip",     cd.tooltip);
	HSTR_PUSH_STRING(L, "texture",     cd.iconname);
	HSTR_PUSH_STRING(L, "cursor",      cd.mouseicon);
	HSTR_PUSH_BOOL(L,   "hidden",      cd.onlyKey);
	HSTR_PUSH_BOOL(L,   "disabled",    cd.disabled);
	HSTR_PUSH_BOOL(L,   "showUnique",  cd.showUnique);
	HSTR_PUSH_BOOL(L,   "onlyTexture", cd.onlyTexture);

	HSTR_PUSH(L, "params");
	lua_newtable(L);
	const int pCount = (int)cd.params.size();
	for (int p = 0; p < pCount; p++) {
		lua_pushnumber(L, p + 1);
		lua_pushstring(L, cd.params[p].c_str());
		lua_rawset(L, -3);
	}
	HSTR_PUSH_NUMBER(L, "n", pCount);
	lua_rawset(L, -3);
}


int CLuaUI::GetActiveCmdDescs(lua_State* L)
{
	if (guihandler == NULL) {
		return 0;
	}
	CheckNoArgs(L, __FUNCTION__);
	lua_newtable(L);
	const vector<CommandDescription>& cmdDescs = guihandler->commands;
	const int cmdDescCount = (int)cmdDescs.size();
	for (int i = 0; i < cmdDescCount; i++) {
		lua_pushnumber(L, i + CMD_INDEX_OFFSET);
		PushCommandDesc(L, cmdDescs[i]);
		lua_rawset(L, -3);
	}
	HSTR_PUSH_NUMBER(L, "n", cmdDescCount);
	return 1;
}


int CLuaUI::GetActiveCmdDesc(lua_State* L)
{
	if (guihandler == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetActiveCmdDesc()");
	}
	const int cmdIndex = (int)lua_tonumber(L, 1) - CMD_INDEX_OFFSET;

	const vector<CommandDescription>& cmdDescs = guihandler->commands;
	const int cmdDescCount = (int)cmdDescs.size();
	if ((cmdIndex < 0) || (cmdIndex >= cmdDescCount)) {
		return 0;
	}
	PushCommandDesc(L, cmdDescs[cmdIndex]);
	return 1;
}


int CLuaUI::GetCmdDescIndex(lua_State* L)
{
	if (guihandler == NULL) {
		return 0;
	}
	CheckNoArgs(L, __FUNCTION__);
	const int cmdId = (int)lua_tonumber(L, 1);
	
	const vector<CommandDescription>& cmdDescs = guihandler->commands;
	const int cmdDescCount = (int)cmdDescs.size();
	for (int i = 0; i < cmdDescCount; i++) {
		if (cmdId == cmdDescs[i].id) {
			lua_pushnumber(L, i + CMD_INDEX_OFFSET);
			return 1;
		}
	}
	return 0;
}


/******************************************************************************/

int CLuaUI::GetMouseState(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, mouse->lastx - gu->viewPosX);
	lua_pushnumber(L, gu->viewSizeY - mouse->lasty - 1);
	lua_pushboolean(L, mouse->buttons[SDL_BUTTON_LEFT].pressed);
	lua_pushboolean(L, mouse->buttons[SDL_BUTTON_MIDDLE].pressed);
	lua_pushboolean(L, mouse->buttons[SDL_BUTTON_RIGHT].pressed);
	return 5;
}


int CLuaUI::WarpMouse(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to WarpMouse()");
	}
	const int x = (int)lua_tonumber(L, 1);
	const int y = gu->viewSizeY - (int)lua_tonumber(L, 2) - 1;
	mouse->WarpMouse(x, y);
	return 0;
}


int CLuaUI::GetMouseCursor(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushstring(L, mouse->cursorText.c_str());
	lua_pushnumber(L, mouse->cursorScale);
	return 2;
}


int CLuaUI::SetMouseCursor(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1) ||
	    ((args >= 2) && !lua_isnumber(L, 2))) {
		luaL_error(L,
			"Incorrect arguments to SetMouseCursor(\"name\", [scale])");
	}
	mouse->cursorText = lua_tostring(L, 1);
	if (args >= 2) {
		mouse->cursorScale = lua_tonumber(L, 2);
	}
	return 0;
}


int CLuaUI::AddMouseCursor(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isstring(L, 1) || !lua_isstring(L, 2)) {
		luaL_error(L, "Incorrect arguments to AddMouseCursor()");
	}

	const string cursorName = lua_tostring(L, 1);
	const string fileName   = lua_tostring(L, 2);

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

	if (mouse->AddMouseCursor(cursorName, fileName, hotSpot, overwrite)) {
		lua_pushboolean(L, true);
	} else {
		lua_pushboolean(L, false);
	}
	return 1;
}


/******************************************************************************/
/******************************************************************************/

int CLuaUI::GetKeyState(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetKeyState(keycode)");
	}
	const int key = (int)lua_tonumber(L, 1);
	if ((key < 0) || (key >= SDLK_LAST)) {
		lua_pushboolean(L, 0);
	} else {
		lua_pushboolean(L, keys[key]);
	}
	return 1;
}


int CLuaUI::GetModKeyState(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushboolean(L, keys[SDLK_LALT]);
	lua_pushboolean(L, keys[SDLK_LCTRL]);
	lua_pushboolean(L, keys[SDLK_LMETA]);
	lua_pushboolean(L, keys[SDLK_LSHIFT]);
	return 4;
}


int CLuaUI::GetPressedKeys(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_newtable(L);
	int count = 0;
	for (int i = 0; i < SDLK_LAST; i++) {
		if (keys[i]) {
			lua_pushnumber(L, i);
			lua_pushboolean(L, 1);
			lua_rawset(L, -3);
			count++;
		}
	}
	lua_pushstring(L, "n");
	lua_pushnumber(L, count);
	lua_rawset(L, -3);
	return 1;
}


int CLuaUI::GetConsoleBuffer(lua_State* L)
{
	CInfoConsole* ic = game->infoConsole;
	if (ic == NULL) {
		return true;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args != 0) && ((args != 1) || !lua_isnumber(L, 1))) {
		luaL_error(L, "Incorrect arguments to GetConsoleBuffer([count])");
	}

	deque<CInfoConsole::RawLine> lines;
	const int newLines = ic->GetRawLines(lines);
	const int lineCount = (int)lines.size();

	int start = 0;
	if (args >= 1) {
		const int maxLines = (int)lua_tonumber(L, 1);
		if (maxLines < lineCount) {
			start = (lineCount - maxLines);
		}
	}

	// table = { [1] = { text = string, priority = number}, etc... }
	lua_newtable(L);
	int count = 0;
	for (int i = start; i < lineCount; i++) {
		count++;
		lua_pushnumber(L, count);
		lua_newtable(L); {
			lua_pushstring(L, "text");
			lua_pushstring(L, lines[i].text.c_str());
			lua_rawset(L, -3);
			lua_pushstring(L, "priority");
			lua_pushnumber(L, lines[i].priority);
			lua_rawset(L, -3);
		}
		lua_rawset(L, -3);
	}
	lua_pushstring(L, "n");
	lua_pushnumber(L, count);
	lua_rawset(L, -3);

	return 1;
}


int CLuaUI::GetCurrentTooltip(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	const string tooltip = mouse->GetCurrentTooltip();
	lua_pushstring(L, tooltip.c_str());
	return 1;
}


int CLuaUI::GetKeyCode(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetKeyCode(\"keysym\")");
	}
	const string keysym = lua_tostring(L, 1);
	lua_pushnumber(L, keyCodes->GetCode(keysym));
	return 1;
}


int CLuaUI::GetKeySymbol(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetKeySymbol(keycode)");
	}
	const int keycode = (int)lua_tonumber(L, 1);
	lua_pushstring(L, keyCodes->GetName(keycode).c_str());
	lua_pushstring(L, keyCodes->GetDefaultName(keycode).c_str());
	return 2;
}


int CLuaUI::GetKeyBindings(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetKeyBindings(\"keyset\")");
	}
	const string keysetStr = lua_tostring(L, 1);
	CKeySet ks;
	if (!ks.Parse(keysetStr)) {
		return 0;
	}
	const CKeyBindings::ActionList&	actions = keyBindings->GetActionList(ks);
	lua_newtable(L);
	for (int i = 0; i < (int)actions.size(); i++) {
		const CKeyBindings::Action& action = actions[i];
		lua_pushnumber(L, i + 1);
		lua_newtable(L);
		lua_pushstring(L, action.command.c_str());
		lua_pushstring(L, action.extra.c_str());
		lua_rawset(L, -3);
		lua_rawset(L, -3);
	}
	lua_pushstring(L, "n");
	lua_pushnumber(L, actions.size());
	lua_rawset(L, -3);
	return 1;
}


int CLuaUI::GetActionHotKeys(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetActionHotKeys(\"command\")");
	}
	const string command = lua_tostring(L, 1);
	const CKeyBindings::HotkeyList&	hotkeys = keyBindings->GetHotkeys(command);
	lua_newtable(L);
	for (int i = 0; i < (int)hotkeys.size(); i++) {
		const string& hotkey = hotkeys[i];
		lua_pushnumber(L, i + 1);
		lua_pushstring(L, hotkey.c_str());
		lua_rawset(L, -3);
	}
	lua_pushstring(L, "n");
	lua_pushnumber(L, hotkeys.size());
	lua_rawset(L, -3);
	return 1;
}


int CLuaUI::GetConfigInt(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || (args > 2) || !lua_isstring(L, 1) ||
	    ((args == 2) && !lua_isnumber(L, 2))) {
		luaL_error(L, "Incorrect arguments to GetConfigInt()");
	}
	const string name = lua_tostring(L, 1);
	int def = 0;
	if (args == 2) {
		def = (int)lua_tonumber(L, 2);
	}
	const int value = configHandler.GetInt(name, def);
	lua_pushnumber(L, value);
	return 1;
}


int CLuaUI::SetConfigInt(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isstring(L, 1) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetConfigInt()");
	}
	const string name = lua_tostring(L, 1);
	const int value = (int)lua_tonumber(L, 2);
	configHandler.SetInt(name, value);
	return 0;
}


int CLuaUI::GetConfigString(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || (args > 2) || !lua_isstring(L, 1) ||
	    ((args == 2) && !lua_isstring(L, 2))) {
		luaL_error(L, "Incorrect arguments to GetConfigString()");
	}
	const string name = lua_tostring(L, 1);
	string def;
	if (args == 2) {
		def = lua_tostring(L, 2);
	}
	const string value = configHandler.GetString(name, def);
	lua_pushstring(L, value.c_str());
	return 1;
}


int CLuaUI::SetConfigString(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isstring(L, 1) || !lua_isstring(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetConfigString()");
	}
	const string name = lua_tostring(L, 1);
	const string value = lua_tostring(L, 2);
	configHandler.SetString(name, value);
	return 0;
}


/******************************************************************************/

int CLuaUI::MakeFont(lua_State* L)
{
	if (!CLuaHandle::GetActiveHandle()->GetUserMode()) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to gl.MakeFont()");
	}
	FontTexture::Reset();
	FontTexture::SetInFileName(lua_tostring(L, 1));
	if ((args >= 2) && lua_istable(L, 2)) {
		const int table = 2;
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (lua_isstring(L, -2)) {
				const string key = lua_tostring(L, -2);
				if (lua_isstring(L, -1)) {
					if ((key == "outName") && lua_isstring(L, -1)) {
						FontTexture::SetOutBaseName(lua_tostring(L, -1));
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
	FontTexture::Execute();
	return 0;
}


/******************************************************************************/

int CLuaUI::SetUnitDefIcon(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
		luaL_error(L,
			"Incorrect arguments to SetUnitDefIcon(unitDefID, \"icon\")");
	}
	const int unitDefID = (int)lua_tonumber(L, 1);
	UnitDef* ud = unitDefHandler->GetUnitByID(unitDefID);
	if (ud == NULL) {
		return 0;
	}

	ud->iconType = lua_tostring(L, 2);

	// set decoys to the same icon
	const map<UnitDef*, set<UnitDef*> >& decoyMap = unitDefHandler->decoyMap;
	map<UnitDef*, set<UnitDef*> >::const_iterator fit;
	if (ud->decoyDef) {
		fit = decoyMap.find(ud->decoyDef);
		ud->decoyDef->iconType = ud->iconType;
	} else {
		fit = decoyMap.find(ud);
	}
	if (fit != decoyMap.end()) {
		const set<UnitDef*>& decoySet = fit->second;
		set<UnitDef*>::const_iterator dit;
		for (dit = decoySet.begin(); dit != decoySet.end(); ++dit) {
			(*dit)->iconType = ud->iconType;
		}
	}

	return 0;
}


/******************************************************************************/

int CLuaUI::GetGroupList(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	if (grouphandler == NULL) {
		return 0;
	}
	lua_newtable(L);
	int count = 0;
	const vector<CGroup*>& groups = grouphandler->groups;
	vector<CGroup*>::const_iterator git;
	for (git = groups.begin(); git != groups.end(); ++git) {
		const CGroup* group = *git;
		if ((group != NULL) && !group->units.empty()) {
			lua_pushnumber(L, group->id);
			lua_pushnumber(L, group->units.size());
			lua_rawset(L, -3);
			count++;
		}
	}
	lua_pushnumber(L, count);
	return 2;
}


int CLuaUI::GetSelectedGroup(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, selectedUnits.selectedGroup);
	return 1;
}


int CLuaUI::GetGroupAIList(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_newtable(L);
	const map<AIKey, string>& availableAI = grouphandler->availableAI;
	map<AIKey, string>::const_iterator it;
	int count = 0;
	for (it = availableAI.begin(); it != availableAI.end(); ++it) {
		count++;
		lua_pushnumber(L, count);
		lua_pushstring(L, it->second.c_str());
		lua_rawset(L, -3);
	}
	lua_pushstring(L, "n");
	lua_pushnumber(L, count);
	lua_rawset(L, -3);
	return 1;
}


int CLuaUI::GetGroupAIName(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetGroupAIName(groupID)");
	}

	const int groupID = (int)lua_tonumber(L, 1);
	if ((groupID < 0) || (groupID >= (int)grouphandler->groups.size())) {
		return 0; // bad group
	}

	const CGroup* group = grouphandler->groups[groupID];
	if (group->ai == NULL) {
		lua_pushstring(L, "");
		return 1;
	}

	const AIKey& aikey = group->currentAiKey;
	const map<AIKey, string>& availableAI = grouphandler->availableAI;
	map<AIKey, string>::const_iterator fit = availableAI.find(aikey);
	if (fit == availableAI.end()) {
		lua_pushstring(L, ""); // should not happen?
	} else {
		lua_pushstring(L, fit->second.c_str());
	}
	return 1;
}


int CLuaUI::GetUnitGroup(lua_State* L)
{
	CUnit* unit = ParseRawUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	if ((unit->team == gu->myTeam) && (unit->group)) {
		lua_pushnumber(L, unit->group->id);
		return 1;
	}
	return 0;
}


int CLuaUI::SetUnitGroup(lua_State* L)
{
	CUnit* unit = ParseRawUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L);
	if ((args < 1) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetUnitGroup()");
	}
	const int groupID = (int)lua_tonumber(L, 2);

	if (groupID == -1) {
		unit->SetGroup(NULL);
		return 0;
	}

	const vector<CGroup*>& groups = grouphandler->groups;
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

int CLuaUI::GetGroupUnits(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetGroupUnits(groupID)");
	}
	const int groupID = (int)lua_tonumber(L, 1);
	const vector<CGroup*>& groups = grouphandler->groups;
	if ((groupID < 0) || (groupID >= groups.size()) ||
	    (groups[groupID] == NULL)) {
		return 0; // nils
	}

	lua_newtable(L);
	int count = 0;
	const set<CUnit*>& groupUnits = groups[groupID]->units;
	set<CUnit*>::const_iterator it;
	for (it = groupUnits.begin(); it != groupUnits.end(); ++it) {
		count++;
		lua_pushnumber(L, count);
		lua_pushnumber(L, (*it)->id);
		lua_rawset(L, -3);
	}
	HSTR_PUSH_NUMBER(L, "n", count);

	return 1;
}


int CLuaUI::GetGroupUnitsSorted(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetGroupUnitsSorted(groupID)");
	}
	const int groupID = (int)lua_tonumber(L, 1);
	const vector<CGroup*>& groups = grouphandler->groups;
	if ((groupID < 0) || (groupID >= groups.size()) ||
	    (groups[groupID] == NULL)) {
		return 0; // nils
	}

	map<int, vector<CUnit*> > unitDefMap;
	const set<CUnit*>& groupUnits = groups[groupID]->units;
	set<CUnit*>::const_iterator it;
	for (it = groupUnits.begin(); it != groupUnits.end(); ++it) {
		CUnit* unit = *it;
		unitDefMap[unit->unitDef->id].push_back(unit);
	}

	lua_newtable(L);
	map<int, vector<CUnit*> >::const_iterator mit;
	for (mit = unitDefMap.begin(); mit != unitDefMap.end(); mit++) {
		lua_pushnumber(L, mit->first); // push the UnitDef index
		lua_newtable(L); {
			const vector<CUnit*>& v = mit->second;
			for (int i = 0; i < (int)v.size(); i++) {
				CUnit* unit = v[i];
				lua_pushnumber(L, i + 1);
				lua_pushnumber(L, unit->id);
				lua_rawset(L, -3);
			}
			HSTR_PUSH_NUMBER(L, "n", v.size());
		}
		lua_rawset(L, -3);
	}
	HSTR_PUSH_NUMBER(L, "n", unitDefMap.size());

	return 1;
}


int CLuaUI::GetGroupUnitsCounts(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		luaL_error(L, "Incorrect arguments to GetGroupUnitsCounts(groupID)");
	}
	const int groupID = (int)lua_tonumber(L, 1);
	const vector<CGroup*>& groups = grouphandler->groups;
	if ((groupID < 0) || (groupID >= groups.size()) ||
	    (groups[groupID] == NULL)) {
		return 0; // nils
	}

	map<int, int> countMap;
	const set<CUnit*>& groupUnits = groups[groupID]->units;
	set<CUnit*>::const_iterator it;
	for (it = groupUnits.begin(); it != groupUnits.end(); ++it) {
		CUnit* unit = *it;
		const int udID = unit->unitDef->id;
		map<int, int>::iterator mit = countMap.find(udID);
		if (mit == countMap.end()) {
			countMap[udID] = 1;
		} else {
			mit->second++;
		}
	}

	lua_newtable(L);
	map<int, int>::const_iterator mit;
	for (mit = countMap.begin(); mit != countMap.end(); mit++) {
		lua_pushnumber(L, mit->first);  // push the UnitDef index
		lua_pushnumber(L, mit->second); // push the UnitDef unit count
		lua_rawset(L, -3);
	}
	HSTR_PUSH_NUMBER(L, "n", countMap.size());

	return 1;
}


/******************************************************************************/
/******************************************************************************/

int CLuaUI::GetMyPlayerID(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, gu->myPlayerNum);
	return 1;
}


int CLuaUI::GetMyTeamID(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, gu->myTeam);
	return 1;
}


int CLuaUI::GetMyAllyTeamID(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, gu->myAllyTeam);
	return 1;
}


/******************************************************************************/
/******************************************************************************/

static void ParseCommandOptions(lua_State* L, const char* caller,
                                int index, Command& cmd)
{
	if (lua_isnumber(L, index)) {
		cmd.options = (unsigned char)lua_tonumber(L, index);
	}
	else if (lua_istable(L, index)) {
		const int optionTable = index;
		for (lua_pushnil(L); lua_next(L, optionTable) != 0; lua_pop(L, 1)) {
			if (lua_isnumber(L, -2)) { // avoid 'n'
				if (!lua_isstring(L, -1)) {
					luaL_error(L, "%s(): bad option table entry", caller);
				}
				const string value = lua_tostring(L, -1);
				if (value == "right") {
					cmd.options |= RIGHT_MOUSE_KEY;
				} else if (value == "alt") {
					cmd.options |= ALT_KEY;
				} else if (value == "ctrl") {
					cmd.options |= CONTROL_KEY;
				} else if (value == "shift") {
					cmd.options |= SHIFT_KEY;
				}
			}
		}
	}
	else {
		luaL_error(L, "%s(): bad options", caller);
	}
}


static void ParseCommand(lua_State* L, const char* caller,
                         int idIndex, Command& cmd)
{
	// cmdID
	if (!lua_isnumber(L, idIndex)) {
		luaL_error(L, "%s(): bad command ID", caller);
	}
	cmd.id = (int)lua_tonumber(L, idIndex);

	// params
	const int paramTable = (idIndex + 1);
	if (!lua_istable(L, paramTable)) {
		luaL_error(L, "%s(): bad param table", caller);
	}
	for (lua_pushnil(L); lua_next(L, paramTable) != 0; lua_pop(L, 1)) {
		if (lua_isnumber(L, -2)) { // avoid 'n'
			if (!lua_isnumber(L, -1)) {
				luaL_error(L, "%s(): bad param table entry", caller);
			}
			const float value = (float)lua_tonumber(L, -1);
			cmd.params.push_back(value);
		}
	}

	// options
	ParseCommandOptions(L, caller, (idIndex + 2), cmd);

	// NOTE: should do some sanity checking?
}


static void ParseCommandTable(lua_State* L, const char* caller,
                              int table, Command& cmd)
{
	// cmdID
	lua_rawgeti(L, table, 1);
	if (!lua_isnumber(L, -1)) {
		luaL_error(L, "%s(): bad command ID", caller);
	}
	cmd.id = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);

	// params
	lua_rawgeti(L, table, 2);
	if (!lua_istable(L, -1)) {
		luaL_error(L, "%s(): bad param table", caller);
	}
	const int paramTable = lua_gettop(L);
	for (lua_pushnil(L); lua_next(L, paramTable) != 0; lua_pop(L, 1)) {
		if (lua_isnumber(L, -2)) { // avoid 'n'
			if (!lua_isnumber(L, -1)) {
				luaL_error(L, "%s(): bad param table entry", caller);
			}
			const float value = (float)lua_tonumber(L, -1);
			cmd.params.push_back(value);
		}
	}
	lua_pop(L, 1);

	// options
	lua_rawgeti(L, table, 3);
	ParseCommandOptions(L, caller, lua_gettop(L), cmd);
	lua_pop(L, 1);

	// NOTE: should do some sanity checking?
}


static void ParseCommandArray(lua_State* L, const char* caller,
                              int table, vector<Command>& commands)
{
	if (!lua_istable(L, table)) {
		luaL_error(L, "%s(): error parsing command array", caller);
	}
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_istable(L, -1)) {
			continue;
		}
		Command cmd;
		ParseCommandTable(L, caller, lua_gettop(L), cmd);
		commands.push_back(cmd);
	}
}


static void ParseUnitMap(lua_State* L, const char* caller,
                         int table, vector<int>& unitIDs)
{
	if (!lua_istable(L, table)) {
		luaL_error(L, "%s(): error parsing unit map", caller);
	}
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_isnumber(L, -2)) {
			CUnit* unit = ParseCtrlUnit(L, __FUNCTION__, -2); // the key
			if (unit != NULL) {
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
		if (lua_isnumber(L, -2) && lua_isnumber(L, -1)) {   // avoid 'n'
			CUnit* unit = ParseCtrlUnit(L, __FUNCTION__, -1); // the value
			if (unit != NULL) {
				unitIDs.push_back(unit->id);
			}
		}
	}
	return;
}


/******************************************************************************/

int CLuaUI::GiveOrder(lua_State* L)
{
	if (gu->spectating || gs->noHelperAIs || (gs->frameNum <= 0)) {
		lua_pushboolean(L, false);
		return 1;
	}

	Command cmd;
	ParseCommand(L, __FUNCTION__, 1, cmd);

	selectedUnits.GiveCommand(cmd);

	lua_pushboolean(L, true);

	return 1;
}


int CLuaUI::GiveOrderToUnit(lua_State* L)
{
	if (gu->spectating || gs->noHelperAIs || (gs->frameNum <= 0)) {
		lua_pushboolean(L, false);
		return 1;
	}

	CUnit* unit = ParseCtrlUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		lua_pushboolean(L, false);
		return 1;
	}

	Command cmd;
	ParseCommand(L, __FUNCTION__, 2, cmd);

	net->SendAICommand(gu->myPlayerNum,
	                   unit->id, cmd.id, cmd.options, cmd.params);

	lua_pushboolean(L, true);
	return 1;
}


int CLuaUI::GiveOrderToUnitMap(lua_State* L)
{
	if (gu->spectating || gs->noHelperAIs || (gs->frameNum <= 0)) {
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
	ParseCommand(L, __FUNCTION__, 2, cmd);

	vector<Command> commands;
	commands.push_back(cmd);
	selectedUnits.SendCommandsToUnits(unitIDs, commands);

	lua_pushboolean(L, true);
	return 1;
}


int CLuaUI::GiveOrderToUnitArray(lua_State* L)
{
	if (gu->spectating || gs->noHelperAIs || (gs->frameNum <= 0)) {
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
	ParseCommand(L, __FUNCTION__, 2, cmd);

	vector<Command> commands;
	commands.push_back(cmd);
	selectedUnits.SendCommandsToUnits(unitIDs, commands);

	lua_pushboolean(L, true);
	return 1;
}


int CLuaUI::GiveOrderArrayToUnitMap(lua_State* L)
{
	if (gu->spectating || gs->noHelperAIs || (gs->frameNum <= 0)) {
		lua_pushboolean(L, false);
		return 1;
	}

	// unitIDs
	vector<int> unitIDs;
	ParseUnitMap(L, __FUNCTION__, 1, unitIDs);

	// commands
	vector<Command> commands;
	ParseCommandArray(L, __FUNCTION__, 2, commands);

	if ((unitIDs.size() <= 0) || (commands.size() <= 0)) {
		lua_pushboolean(L, false);
		return 1;
	}

	selectedUnits.SendCommandsToUnits(unitIDs, commands);

	lua_pushboolean(L, true);
	return 1;
}


int CLuaUI::GiveOrderArrayToUnitArray(lua_State* L)
{
	if (gu->spectating || gs->noHelperAIs || (gs->frameNum <= 0)) {
		lua_pushboolean(L, false);
		return 1;
	}

	// unitIDs
	vector<int> unitIDs;
	ParseUnitArray(L, __FUNCTION__, 1, unitIDs);

	// commands
	vector<Command> commands;
	ParseCommandArray(L, __FUNCTION__, 2, commands);

	if ((unitIDs.size() <= 0) || (commands.size() <= 0)) {
		lua_pushboolean(L, false);
		return 1;
	}

	selectedUnits.SendCommandsToUnits(unitIDs, commands);

	lua_pushboolean(L, true);
	return 1;
}


/******************************************************************************/

int CLuaUI::SetShareLevel(lua_State* L)
{
	if (gu->spectating || (gs->frameNum <= 0)) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isstring(L, 1) || !lua_isnumber(L, 2)) {
		luaL_error(L, "Incorrect arguments to SetShareLevel(\"type\", level");
	}

	const string shareType = lua_tostring(L, 1);
	const float shareLevel = max(0.0f, min(1.0f, (float)lua_tonumber(L, 2)));

	if (shareType == "metal") {
		net->SendSetShare(gu->myTeam, shareLevel, gs->Team(gu->myTeam)->energyShare);
	}
	else if (shareType == "energy") {
		net->SendSetShare(gu->myTeam, gs->Team(gu->myTeam)->metalShare, shareLevel);
	}
	else {
		logOutput.Print("SetShareLevel() unknown resource: %s", shareType.c_str());
	}
	return 0;
}


int CLuaUI::ShareResources(lua_State* L)
{
	if (gu->spectating || (gs->frameNum <= 0)) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isstring(L, 2) ||
	    ((args >= 3) && !lua_isnumber(L, 3))) {
		luaL_error(L, "Incorrect arguments to ShareResources()");
	}
	const int teamID = (int)lua_tonumber(L, 1);
	if ((teamID < 0) || (teamID >= gs->activeTeams)) {
		return 0;
	}
	const CTeam* team = gs->Team(teamID);
	if ((team == NULL) || team->isDead) {
		return 0;
	}
	const string& type = lua_tostring(L, 2);
	if (type == "units") {
		// update the selection, and clear the unit command queues
		Command c;
		c.id = CMD_STOP;
		selectedUnits.GiveCommand(c, false);
		net->SendShare(gu->myPlayerNum, teamID, 1, 0.0f, 0.0f);
		selectedUnits.ClearSelected();
	}
	else if (args >= 3) {
		const float amount = (float)lua_tonumber(L, 3);
		if (type == "metal") {
			net->SendShare(gu->myPlayerNum, teamID, 0, amount, 0.0f);
		}
		else if (type == "energy") {
			net->SendShare(gu->myPlayerNum, teamID, 0, 0.0f, amount);
		}
	}
	return 0;
}


/******************************************************************************/
/******************************************************************************/

int CLuaUI::MarkerAddPoint(lua_State* L)
{
	if (inMapDrawer == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 3) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2)  || !lua_isnumber(L, 3) ||
	    ((args >= 4) && !lua_isstring(L, 4))) {
		luaL_error(L, "Incorrect arguments to MarkerAddPoint(x, y, z[, text])");
	}
	const float3 pos((float)lua_tonumber(L, 1),
	                 (float)lua_tonumber(L, 2),
	                 (float)lua_tonumber(L, 3));
	string text = "";
	if (args >= 4) {
	  text = lua_tostring(L, 4);
	}

	inMapDrawer->CreatePoint(pos, text);

	return 0;
}


int CLuaUI::MarkerAddLine(lua_State* L)
{
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
	const float3 pos1((float)lua_tonumber(L, 1),
	                  (float)lua_tonumber(L, 2),
	                  (float)lua_tonumber(L, 3));
	const float3 pos2((float)lua_tonumber(L, 4),
	                  (float)lua_tonumber(L, 5),
	                  (float)lua_tonumber(L, 6));

	inMapDrawer->AddLine(pos1, pos2);

	return 0;
}


int CLuaUI::MarkerErasePosition(lua_State* L)
{
	if (inMapDrawer == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args != 3) ||
	    !lua_isstring(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
		luaL_error(L, "Incorrect arguments to MarkerDeletePositionl(x, y, z)");
	}
	const float3 pos((float)lua_tonumber(L, 1),
	                 (float)lua_tonumber(L, 2),
	                 (float)lua_tonumber(L, 3));

	inMapDrawer->ErasePos(pos);

	return 0;
}


/******************************************************************************/

/******************************************************************************/
/******************************************************************************/
