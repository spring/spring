/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifdef _MSC_VER
#include "StdAfx.h"
#endif
#include "Lua/LuaLobby.h" // ugh, streflop namespace corruption...
#ifndef _MSC_VER
#include "StdAfx.h"
#endif

#include <stdio.h>
#include <set>
#include <cctype>
#include <SDL_keysym.h>
#include <SDL_mouse.h>
#include <SDL_timer.h>

#include "mmgr.h"

#include "LuaUI.h"

#include "LuaInclude.h"

#include "Lua/LuaUnsyncedCtrl.h"
#include "Lua/LuaCallInCheck.h"
#include "Lua/LuaUtils.h"
#include "Lua/LuaConstGL.h"
#include "Lua/LuaConstCMD.h"
#include "Lua/LuaConstCMDTYPE.h"
#include "Lua/LuaConstGame.h"
#include "Lua/LuaSyncedRead.h"
#include "Lua/LuaUnsyncedCall.h"
#include "Lua/LuaUnsyncedRead.h"
#include "Lua/LuaFeatureDefs.h"
#include "Lua/LuaUnitDefs.h"
#include "Lua/LuaWeaponDefs.h"
#include "Lua/LuaScream.h"
#include "Lua/LuaOpenGL.h"
#include "Lua/LuaVFS.h"
#include "Lua/LuaIO.h"
#include "Lua/LuaZip.h"
#include "Game/Camera.h"
#include "Game/Camera/CameraController.h"
#include "Game/Game.h"
#include "Game/GameHelper.h"
#include "Game/PlayerRoster.h"
#include "Game/SelectedUnits.h"
#include "CommandColors.h"
#include "CursorIcons.h"
#include "GuiHandler.h"
#include "InfoConsole.h"
#include "KeyCodes.h"
#include "KeySet.h"
#include "KeyBindings.h"
#include "MiniMap.h"
#include "MouseHandler.h"
#include "Map/ReadMap.h"
#include "Rendering/IconHandler.h"
#include "Rendering/InMapDraw.h"
#include "EventHandler.h"
#include "LogOutput.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/VFSHandler.h"
#include "ConfigHandler.h"
#include "FileSystem/FileSystem.h"
#include "Util.h"

using namespace std;

CLuaUI* luaUI = NULL;

const int CMD_INDEX_OFFSET = 1; // starting index for command descriptions


/******************************************************************************/
/******************************************************************************/


void CLuaUI::LoadHandler()
{
	if (luaUI) {
		return;
	}

	new CLuaUI();

	if (luaUI->L == NULL) {
		delete luaUI;
	}
}


void CLuaUI::FreeHandler()
{
	static bool inFree = false;
	if (!inFree) {
		inFree = true;
		delete luaUI;
		inFree = false;
	}
}


/******************************************************************************/

CLuaUI::CLuaUI()
: CLuaHandle("LuaUI", LUA_HANDLE_ORDER_UI, true)
{
	luaUI = this;

	if (L == NULL) {
		return;
	}

	UpdateTeams();

	haveShockFront = false;
	shockFrontMinArea  = 0.0f;
	shockFrontMinPower = 0.0f;
	shockFrontDistAdj  = 100.0f;

	const string code = LoadFile("luaui.lua");
	if (code.empty()) {
		KillLua();
		return;
	}

	// load the standard libraries
	LUA_OPEN_LIB(L, luaopen_base);
	LUA_OPEN_LIB(L, luaopen_io);
	LUA_OPEN_LIB(L, luaopen_os);
	LUA_OPEN_LIB(L, luaopen_math);
	LUA_OPEN_LIB(L, luaopen_table);
	LUA_OPEN_LIB(L, luaopen_string);
	//LUA_OPEN_LIB(L, luaopen_package);
	LUA_OPEN_LIB(L, luaopen_debug);

	// setup the lua IO access check functions
	lua_set_fopen(L, LuaIO::fopen);
	lua_set_popen(L, LuaIO::popen, LuaIO::pclose);
	lua_set_system(L, LuaIO::system);
	lua_set_remove(L, LuaIO::remove);
	lua_set_rename(L, LuaIO::rename);

	// remove a few dangerous calls
	lua_getglobal(L, "io");
	lua_pushstring(L, "popen"); lua_pushnil(L); lua_rawset(L, -3);
	lua_pop(L, 1);
	lua_getglobal(L, "os"); {
		lua_pushliteral(L, "exit");      lua_pushnil(L); lua_rawset(L, -3);
		lua_pushliteral(L, "execute");   lua_pushnil(L); lua_rawset(L, -3);
		//lua_pushliteral(L, "remove");    lua_pushnil(L); lua_rawset(L, -3);
		//lua_pushliteral(L, "rename");    lua_pushnil(L); lua_rawset(L, -3);
		lua_pushliteral(L, "tmpname");   lua_pushnil(L); lua_rawset(L, -3);
		lua_pushliteral(L, "getenv");    lua_pushnil(L); lua_rawset(L, -3);
		//lua_pushliteral(L, "setlocale"); lua_pushnil(L); lua_rawset(L, -3);
	}
	lua_pop(L, 1); // os

	lua_pushvalue(L, LUA_GLOBALSINDEX);

	AddBasicCalls(); // into Global

	lua_pushstring(L, "Script");
	lua_rawget(L, -2);
	LuaPushNamedCFunc(L, "UpdateCallIn", CallOutUnsyncedUpdateCallIn);
	lua_pop(L, 1);

	// load the spring libraries
	if (!LoadCFunctions(L)                                                 ||
	    !AddEntriesToTable(L, "VFS",         LuaVFS::PushUnsynced)         ||
	    !AddEntriesToTable(L, "VFS",       LuaZipFileReader::PushUnsynced) ||
	    !AddEntriesToTable(L, "VFS",       LuaZipFileWriter::PushUnsynced) ||
	    !AddEntriesToTable(L, "UnitDefs",    LuaUnitDefs::PushEntries)     ||
	    !AddEntriesToTable(L, "WeaponDefs",  LuaWeaponDefs::PushEntries)   ||
	    !AddEntriesToTable(L, "FeatureDefs", LuaFeatureDefs::PushEntries)  ||
	    !AddEntriesToTable(L, "Script",      LuaUnsyncedCall::PushEntries) ||
	    !AddEntriesToTable(L, "Script",      LuaScream::PushEntries)       ||
	    !AddEntriesToTable(L, "Script",      LuaLobby::PushEntries)        ||
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
	if (!LoadCode(code, "luaui.lua")) {
		KillLua();
		return;
	}

	// register for call-ins
	eventHandler.AddClient(this);

	// update extra call-ins
	UnsyncedUpdateCallIn("WorldTooltip");
	UnsyncedUpdateCallIn("MapDrawCmd");

	lua_settop(L, 0);
}


CLuaUI::~CLuaUI()
{
	if (L != NULL) {
		Shutdown();
		KillLua();
	}
	luaUI = NULL;
}


string CLuaUI::LoadFile(const string& filename) const
{
	const char* accessMode = SPRING_VFS_RAW;
	CFileHandler lockFile("gamedata/LockLuaUI.txt", SPRING_VFS_MOD);
	if (lockFile.FileExists()) {
		if (!CLuaHandle::GetDevMode()) {
			logOutput.Print("This mod has locked LuaUI access");
			accessMode = SPRING_VFS_MOD;
		} else {
			logOutput.Print("Bypassing this mod's LuaUI access lock");
			accessMode = SPRING_VFS_RAW SPRING_VFS_MOD;
		}
	}

	CFileHandler f(filename, accessMode);

	string code;
	if (!f.LoadStringData(code)) {
		code.clear();
	}
	return code;
}


bool CLuaUI::HasCallIn(const string& name)
{
	if (L == NULL) {
		return false;
	}

	// never allow these calls
	if (name == "Explosion") {
		return false;
	}

	lua_getglobal(L, name.c_str());
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	lua_pop(L, 1);
	return true;
}


bool CLuaUI::UnsyncedUpdateCallIn(const string& name)
{
	// never allow this call-in
	if (name == "Explosion") {
		return false;
	}

	if (HasCallIn(name)) {
		eventHandler.InsertEvent(this, name);
	} else {
		eventHandler.RemoveEvent(this, name);
	}
	return true;
}


void CLuaUI::UpdateTeams()
{
	if (luaUI) {
		luaUI->fullCtrl = gs->godMode;
		luaUI->ctrlTeam = gs->godMode ? AllAccessTeam :
		                  (gu->spectating ? NoAccessTeam : gu->myTeam);
		luaUI->fullRead = gu->spectatingFullView;
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

	REGISTER_LUA_CFUNC(SetShockFrontFactors);

	lua_setglobal(L, "Spring");

	return true;
}


/******************************************************************************/
/******************************************************************************/

void CLuaUI::Shutdown()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);
	static const LuaHashString cmdStr("Shutdown");
	if (!cmdStr.GetGlobalFunc(L)) {
		return;
	}

	// call the routine
	RunCallIn(cmdStr, 0, 0);

	return;
}


bool CLuaUI::ConfigCommand(const string& command)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);
	static const LuaHashString cmdStr("ConfigureLayout");
	if (!cmdStr.GetGlobalFunc(L)) {
		return true; // the call is not defined
	}

	lua_pushstring(L, command.c_str());

	// call the routine
	if (!RunCallIn(cmdStr, 1, 0)) {
		return false;
	}
	return true;
}


bool CLuaUI::AddConsoleLines()
{
	CInfoConsole* ic = game->infoConsole;
	if (ic == NULL) {
		return true;
	}

	vector<CInfoConsole::RawLine> lines;
	ic->GetNewRawLines(lines);

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);

	const int count = (int)lines.size();
	for (int i = 0; i < count; i++) {
		const CInfoConsole::RawLine& rl = lines[i];

		static const LuaHashString cmdStr("AddConsoleLine");
		if (!cmdStr.GetGlobalFunc(L)) {
			return true; // the call is not defined
		}

		// FIXME: migrate priority to subsystem...
		lua_pushstring(L, rl.text.c_str());
		lua_pushnumber(L, 0 /*priority*/ );
		//lua_pushstring(L, rl.subsystem->name);

		// call the function
		if (!RunCallIn(cmdStr, 2, 0)) {
			return false;
		}
	}
	return true;
}


static inline float fuzzRand(float fuzz)
{
	return (1.0f + fuzz) - ((2.0f * fuzz) * (float)rand() / (float)RAND_MAX);
}


void CLuaUI::ShockFront(float power, const float3& pos, float areaOfEffect)
{
	if (!haveShockFront) {
		return;
	}
	if (areaOfEffect < shockFrontMinArea) {
		return;
	}

	float3 gap = (camera->pos - pos);
	float dist = gap.Length() + shockFrontDistAdj;

	power = power / (dist * dist);
	if (power < shockFrontMinPower) {
		return;
	}

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 6);
	static const LuaHashString cmdStr("ShockFront");
	if (!cmdStr.GetGlobalFunc(L)) {
		haveShockFront = false;
		return; // the call is not defined
	}

	if (!gu->spectatingFullView && !loshandler->InLos(pos, gu->myAllyTeam)) {
		const float fuzz = 0.25f;
		gap.x *= fuzzRand(fuzz);
		gap.y *= fuzzRand(fuzz);
		gap.z *= fuzzRand(fuzz);
		dist = gap.Length() + shockFrontDistAdj;
	}
	const float3 dir = (gap / dist); // normalize

	lua_pushnumber(L, power);
	lua_pushnumber(L, dir.x);
	lua_pushnumber(L, dir.y);
	lua_pushnumber(L, dir.z);

	// call the routinea
	if (!RunCallIn(cmdStr, 4, 0)) {
		return;
	}

	return;
}


/******************************************************************************/

bool CLuaUI::HasLayoutButtons()
{
	GML_RECMUTEX_LOCK(lua); // HasLayoutButtons

	lua_checkstack(L, 2);

	static const LuaHashString cmdStr("LayoutButtons");
	if (!cmdStr.GetGlobalFunc(L)) {
		return false; // the call is not defined
	}
	lua_pop(L, 1);
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

	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 6);
	const int top = lua_gettop(L);

	static const LuaHashString cmdStr("LayoutButtons");
	if (!cmdStr.GetGlobalFunc(L)) {
		return false;
	}

	lua_pushnumber(L, xButtons);
	lua_pushnumber(L, yButtons);
	lua_pushnumber(L, cmds.size());

	if (!BuildCmdDescTable(L, cmds)) {
		lua_settop(L, top);
		return false;
	}

	// call the function
	if (!RunCallIn(cmdStr, 4, LUA_MULTRET)) {
		lua_settop(L, top);
		return false;
	}

	// get the results
	const int args = lua_gettop(L);

	if (lua_israwstring(L, -1) &&
	    (string(lua_tostring(L, -1)) == "disabled")) {
		lua_settop(L, top);
		return false; // no warnings
	}
	else if (args != 11) {
		logOutput.Print("LayoutButtons() bad number of return values (%i)\n", args);
		lua_settop(L, top);
		return false;
	}

	if (!lua_isstring(L, 1)) {
		logOutput.Print("LayoutButtons() bad menuName value\n");
		lua_settop(L, top);
		return false;
	}
	menuName = string(lua_tostring(L, 1), lua_strlen(L, 1));

	if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
		logOutput.Print("LayoutButtons() bad xButtons or yButtons values\n");
		lua_settop(L, top);
		return false;
	}
	xButtons = lua_toint(L, 2);
	yButtons = lua_toint(L, 3);

	if (!GetLuaIntList(L, 4, removeCmds)) {
		logOutput.Print("LayoutButtons() bad removeCommands table\n");
		lua_settop(L, top);
		return false;
	}

	if (!GetLuaCmdDescList(L, 5, customCmds)) {
		logOutput.Print("LayoutButtons() bad customCommands table\n");
		lua_settop(L, top);
		return false;
	}

	if (!GetLuaIntList(L, 6, onlyTextureCmds)) {
		logOutput.Print("LayoutButtons() bad onlyTexture table\n");
		lua_settop(L, top);
		return false;
	}

	if (!GetLuaReStringList(L, 7, reTextureCmds)) {
		logOutput.Print("LayoutButtons() bad reTexture table\n");
		lua_settop(L, top);
		return false;
	}

	if (!GetLuaReStringList(L, 8, reNamedCmds)) {
		logOutput.Print("LayoutButtons() bad reNamed table\n");
		lua_settop(L, top);
		return false;
	}

	if (!GetLuaReStringList(L, 9, reTooltipCmds)) {
		logOutput.Print("LayoutButtons() bad reTooltip table\n");
		lua_settop(L, top);
		return false;
	}

	if (!GetLuaReParamsList(L, 10, reParamsCmds)) {
		logOutput.Print("LayoutButtons() bad reParams table\n");
		lua_settop(L, top);
		return false;
	}

	if (!GetLuaIntMap(L, 11, buttonList)) {
		logOutput.Print("LayoutButtons() bad buttonList table\n");
		lua_settop(L, top);
		return false;
	}

	lua_settop(L, top);

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
			HSTR_PUSH_STRING(L, "name",     cmds[i].name);
			HSTR_PUSH_STRING(L, "tooltip",  cmds[i].tooltip);
			HSTR_PUSH_STRING(L, "action",   cmds[i].action);
			HSTR_PUSH_STRING(L, "texture",  cmds[i].iconname);
			HSTR_PUSH_BOOL(L,   "hidden",   cmds[i].hidden);
			HSTR_PUSH_BOOL(L,   "disabled", cmds[i].disabled);
			HSTR_PUSH_BOOL(L,"onlyTexture", cmds[i].onlyTexture);

			HSTR_PUSH(L, "params");
			lua_newtable(L);
			for (unsigned int p = 0; p < cmds[i].params.size(); p++) {
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
		if (!lua_isnumber(L, -1) || !lua_israwnumber(L, -2)) {
			lua_pop(L, 2);
			return false;
		}
		const int key = lua_toint(L, -2);
		const int value = lua_toint(L, -1) - CMD_INDEX_OFFSET;
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
		const int value = lua_toint(L, -1) - CMD_INDEX_OFFSET;
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
		if (!lua_israwnumber(L, -2) || !lua_isstring(L, -1)) {
			lua_pop(L, 2);
			return false;
		}
		ReStringPair rp;
		rp.cmdIndex = lua_toint(L, -2) - CMD_INDEX_OFFSET;
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
		if (!lua_israwnumber(L, -2) || !lua_istable(L, -1)) {
			lua_pop(L, 2);
			return false;
		}
		ReParamsPair paramsPair;
		paramsPair.cmdIndex = lua_toint(L, -2) - CMD_INDEX_OFFSET;
		const int paramTable = lua_gettop(L);
		for (lua_pushnil(L); lua_next(L, paramTable) != 0; lua_pop(L, 1)) {
			if (!lua_israwnumber(L, -2) || !lua_isstring(L, -1)) {
				lua_pop(L, 4);
				return false;
			}
			const int paramIndex = lua_toint(L, -2);
			const string paramValue = string(lua_tostring(L, -1), lua_strlen(L, -1));
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
			if (lua_israwstring(L, -2) && lua_isstring(L, -1)) {
				const string key = StringToLower(string(lua_tostring(L, -2), lua_strlen(L, -2)));
				const string value(lua_tostring(L, -1), lua_strlen(L, -1));
				if (key == "id") {
					cd.id = atoi(value.c_str());
				} else if (key == "type") {
					cd.type = atoi(value.c_str());
				} else if (key == "name") {
					cd.name = value;
				} else if (key == "action") {
					cd.action = value;
				} else if (key == "texture") {
					cd.iconname = value;
				} else if (key == "cursor") {
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
			else if (lua_israwstring(L, -2) && lua_istable(L, -1)) {
				const string key = StringToLower(lua_tostring(L, -2));
				if (key != "actions") {
					logOutput.Print("GetLuaCmdDescList() non \"actions\" table: %s",
					                key.c_str());
					continue;
				}
				const int actionsTable = lua_gettop(L);
				for (lua_pushnil(L); lua_next(L, actionsTable) != 0; lua_pop(L, 1)) {
					if (lua_isstring(L, -1)) {
						string action = lua_tostring(L, -1);
						if (action[0] != '@') {
							action = "@@" + action;
						}
						cd.params.push_back(action);
					}
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
	const bool haveFunc = lua_isfunction(L, -1);
	lua_pop(L, 1);
	return haveFunc;
}


int CLuaUI::UnsyncedXCall(lua_State* srcState, const string& funcName)
{
	LUA_CALL_IN_CHECK(L);
	const LuaHashString funcHash(funcName);
	if (!funcHash.GetGlobalFunc(L)) {
		return 0;
	}

	const int top = lua_gettop(L) - 1; // do not count the function

	const bool diffStates = (srcState != L);

	int retCount;

	if (!diffStates) {
		lua_insert(L, 1); // move the function to the beginning
		// call the function
		if (!RunCallIn(funcHash, top, LUA_MULTRET)) {
			return 0;
		}
		retCount = lua_gettop(L);
	}
	else {
		const int srcCount = lua_gettop(srcState);

		LuaUtils::CopyData(L, srcState, srcCount);

		// call the function
		if (!RunCallIn(funcHash, srcCount, LUA_MULTRET)) {
			return 0;
		}
		retCount = lua_gettop(L) - top;

		lua_settop(srcState, 0);
		if (retCount > 0) {
			LuaUtils::CopyData(srcState, L, retCount);
		}
		lua_settop(L, top);
	}

	return retCount;
}


/******************************************************************************/
/******************************************************************************/
//
// Lua Callbacks
//

int CLuaUI::SetShockFrontFactors(lua_State* L)
{
	luaUI->haveShockFront = true;
	if (lua_isnumber(L, 1)) {
		const float value = max(0.0f, lua_tofloat(L, 1));
		luaUI->shockFrontMinArea = value;
	}
	if (lua_isnumber(L, 2)) {
		const float value = max(0.0f, lua_tofloat(L, 2));
		luaUI->shockFrontMinPower = value;
	}
	if (lua_isnumber(L, 3)) {
		const float value = max(1.0f, lua_tofloat(L, 3));
		luaUI->shockFrontDistAdj = value;
	}
	return 0;
}


/******************************************************************************/
/******************************************************************************/
