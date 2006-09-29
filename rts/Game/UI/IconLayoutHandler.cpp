#include "StdAfx.h"
// IconLayoutHandler.cpp: implementation of the CIconLayoutHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "IconLayoutHandler.h"
#include <cctype>
#include "SDL_keysym.h"
extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}
#include "LuaState.h"
#include "ExternalAI/GlobalAIHandler.h"
#include "ExternalAI/Group.h"
#include "ExternalAI/GroupHandler.h"
#include "Game/Game.h"
#include "Game/SelectedUnits.h"
#include "Game/Team.h"
#include "Game/UI/GuiHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "System/Platform/ConfigHandler.h"
#include "System/LogOutput.h"

extern Uint8 *keys;


// LUA C functions  (placed in the Spring table)

static int SendCommands(lua_State* L);

static int GetFPS(lua_State* L);
static int GetInCommand(lua_State* L);

static int GetConfigInt(lua_State* L);
static int SetConfigInt(lua_State* L);
static int GetConfigString(lua_State* L);
static int SetConfigString(lua_State* L);

static int GetSelectedUnits(lua_State* L);
static int GetGroupList(lua_State* L);
static int GetGroupUnits(lua_State* L);

static int GetMyTeamUnits(lua_State* L);
static int GetAlliedUnits(lua_State* L);

static int GetFullBuildQueue(lua_State* L);
static int GetRealBuildQueue(lua_State* L);

static int GetAllyteamList(lua_State* L);
static int GetTeamInfo(lua_State* L);

static int GetPlayerInfo(lua_State* L);
static int GetMyPlayerID(lua_State* L);

static int AreTeamsAllied(lua_State* L);
static int ArePlayersAllied(lua_State* L);


/******************************************************************************/

CIconLayoutHandler* CIconLayoutHandler::GetHandler(const string& filename)
{
	CIconLayoutHandler* layout = new CIconLayoutHandler();

	const string code = layout->LoadFile(filename);
	if (code.empty()) {
		delete layout;
		return NULL;
	}

	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		delete layout;
		return NULL;
	}

	if (!layout->LoadCFunctions(L)) {
		delete layout;
		return NULL;
	}

	if (!layout->LoadCode(L, code, filename)) {
		delete layout;
		return NULL;
	}

	return layout;
}


/******************************************************************************/

CIconLayoutHandler::CIconLayoutHandler()
{
}


CIconLayoutHandler::~CIconLayoutHandler()
{
}


string CIconLayoutHandler::LoadFile(const string& filename)
{
	string code;
	
	FILE* f = fopen(filename.c_str(), "rt");
	if (f == NULL) {
		return code;
	}

	char buf[1024];
	while (fgets(buf, sizeof(buf), f) != NULL) {
		code += buf;
	}
	fclose(f);
	
	return code;
}


bool CIconLayoutHandler::LoadCFunctions(lua_State* L)
{
#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)
	
	lua_newtable(L);

	REGISTER_LUA_CFUNC(SendCommands);
 	REGISTER_LUA_CFUNC(GetFPS);
	REGISTER_LUA_CFUNC(GetInCommand);
	REGISTER_LUA_CFUNC(GetConfigInt);
	REGISTER_LUA_CFUNC(SetConfigInt);
	REGISTER_LUA_CFUNC(GetConfigString);
	REGISTER_LUA_CFUNC(SetConfigString);
	REGISTER_LUA_CFUNC(GetSelectedUnits);
	REGISTER_LUA_CFUNC(GetGroupList);
	REGISTER_LUA_CFUNC(GetGroupUnits);
	REGISTER_LUA_CFUNC(GetMyTeamUnits);
	REGISTER_LUA_CFUNC(GetAlliedUnits);
	REGISTER_LUA_CFUNC(GetFullBuildQueue);
	REGISTER_LUA_CFUNC(GetRealBuildQueue);
	REGISTER_LUA_CFUNC(GetAllyteamList);
	REGISTER_LUA_CFUNC(GetTeamInfo);
	REGISTER_LUA_CFUNC(GetPlayerInfo);
	REGISTER_LUA_CFUNC(GetMyPlayerID);
	REGISTER_LUA_CFUNC(AreTeamsAllied);
	REGISTER_LUA_CFUNC(ArePlayersAllied);

	lua_setglobal(L, "Spring");
	
	return true;
}


bool CIconLayoutHandler::LoadCode(lua_State* L,
                                  const string& code, const string& debug)
{
	int error;
	error = luaL_loadbuffer(L, code.c_str(), code.size(), debug.c_str());
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n", error, debug.c_str(), lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}
	error = lua_pcall(L, 0, 0, 0);
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n", error, debug.c_str(), lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}
	
	return true;
}


bool CIconLayoutHandler::ConfigCommand(const string& command)
{
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}

	lua_pop(L, lua_gettop(L));
	lua_getglobal(L, "ConfigureLayout");
	lua_pushstring(L, command.c_str());
	const int error = lua_pcall(L, 1, 0, 0);
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n", error,
		                "Call_ConfigureLayout", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}
	return true;
}


bool CIconLayoutHandler::UpdateLayout(bool& forceLayout, int activePage)
{
	forceLayout = false;

	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}

	lua_pop(L, lua_gettop(L));
	
	lua_getglobal(L, "UpdateLayout");
	lua_pushnumber(L,  activePage);
	lua_pushboolean(L, keys[SDLK_LALT]);
	lua_pushboolean(L, keys[SDLK_LCTRL]);
	lua_pushboolean(L, keys[SDLK_LMETA]);
	lua_pushboolean(L, keys[SDLK_LSHIFT]);
	const int error = lua_pcall(L, 5, 1, 0);
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n", error,
		                "Call_UpdateLayout", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}
	if (!lua_isboolean(L, -1)) {
		logOutput.Print("UpdateLayout() expects a boolean return value\n");
		return false;
	}
	forceLayout = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return true;
}


bool CIconLayoutHandler::LayoutIcons(int& xIcons, int& yIcons,
                                     const vector<CommandDescription>& cmds,
                                     vector<int>& removeCmds,
                                     vector<CommandDescription>& customCmds,
                                     vector<int>& onlyTextureCmds,
                                     vector<ReStringPair>& reTextureCmds,
                                     vector<ReStringPair>& reNamedCmds,
                                     vector<ReStringPair>& reTooltipCmds,
                                     vector<ReParamsPair>& reParamsCmds,
                                     map<int, int>& iconList,
                                     string& menuName)
{
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	
	customCmds.clear();
	removeCmds.clear();
	reTextureCmds.clear();
	reNamedCmds.clear();
	reTooltipCmds.clear();
	onlyTextureCmds.clear();
	iconList.clear();
	menuName = "";

	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "LayoutIcons");
	lua_pushnumber(L, xIcons);
	lua_pushnumber(L, yIcons);
	lua_pushnumber(L, cmds.size());
	
	if (!BuildCmdDescTable(L, cmds)) {
		lua_pop(L, lua_gettop(L));
		return false;
	}
	
	// call the function
	const int error = lua_pcall(L, 4, LUA_MULTRET, 0);
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n", error,
		                "Call_ConfigureLayout", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}

	// get the results
	const int args = lua_gettop(L);
	if (args != 11) {
		logOutput.Print("LayoutIcons() bad number of return values (%i)\n", args);
		lua_pop(L, args);
		return false;
	}
	
	if (!GetLuaIntMap(L, iconList)) {
		logOutput.Print("LayoutIcons() bad iconList table\n");
		lua_pop(L, lua_gettop(L));
		return false;
	}

	if (!GetLuaReParamsList(L, reParamsCmds)) {
		logOutput.Print("LayoutIcons() bad reParams table\n");
		lua_pop(L, lua_gettop(L));
		return false;
	}

	if (!GetLuaReStringList(L, reTooltipCmds)) {
		logOutput.Print("LayoutIcons() bad reTooltip table\n");
		lua_pop(L, lua_gettop(L));
		return false;
	}

	if (!GetLuaReStringList(L, reNamedCmds)) {
		logOutput.Print("LayoutIcons() bad reNamed table\n");
		lua_pop(L, lua_gettop(L));
		return false;
	}

	if (!GetLuaReStringList(L, reTextureCmds)) {
		logOutput.Print("LayoutIcons() bad reTexture table\n");
		lua_pop(L, lua_gettop(L));
		return false;
	}

	if (!GetLuaIntList(L, onlyTextureCmds)) {
		logOutput.Print("LayoutIcons() bad onlyTexture table\n");
		lua_pop(L, lua_gettop(L));
		return false;
	}

	if (!GetLuaCmdDescList(L, customCmds)) {
		logOutput.Print("LayoutIcons() bad customCommands table\n");
		lua_pop(L, lua_gettop(L));
		return false;
	}

	if (!GetLuaIntList(L, removeCmds)) {
		logOutput.Print("LayoutIcons() bad removeCommands table\n");
		lua_pop(L, lua_gettop(L));
		return false;
	}

	if (!lua_isnumber(L, -1) || !lua_isnumber(L, -2)) {
		logOutput.Print("LayoutIcons() bad xIcons or yIcons values\n");
		lua_pop(L, args);
		return false;
	}
	xIcons = (int)lua_tonumber(L, -2);
	yIcons = (int)lua_tonumber(L, -1);
	lua_pop(L, 2);
	
	if (!lua_isstring(L, -1)) {
		logOutput.Print("LayoutIcons() bad xIcons or yIcons values\n");
		lua_pop(L, args);
		return false;
	}
	menuName = lua_tostring(L, -1);
	lua_pop(L, 1);

	return true;
}


bool CIconLayoutHandler::BuildCmdDescTable(lua_State* L,
                                           const vector<CommandDescription>& cmds)
{
	lua_newtable(L);

	for (int i = 0; i < (int)cmds.size(); i++) {

		lua_pushnumber(L, i);
		lua_newtable(L);

		lua_pushstring(L, "id");
		lua_pushnumber(L, cmds[i].id);
		lua_rawset(L, -3);

		lua_pushstring(L, "type");
		lua_pushnumber(L, cmds[i].type);
		lua_rawset(L, -3);

		lua_pushstring(L, "action");
		lua_pushstring(L, cmds[i].action.c_str());
		lua_rawset(L, -3);

		lua_pushstring(L, "hidden");
		lua_pushboolean(L, cmds[i].onlyKey ? 1 : 0);
		lua_rawset(L, -3);
		
		lua_pushstring(L, "params");
		lua_newtable(L);
		for (int p = 0; p < cmds[i].params.size(); p++) {
			lua_pushnumber(L, p + 1);
			lua_pushstring(L, cmds[i].params[p].c_str());
			lua_rawset(L, -3);
		}
		lua_rawset(L, -3);

		lua_rawset(L, -3);
	}

	return true;
}


bool CIconLayoutHandler::GetLuaIntMap(lua_State* L, map<int, int>& intMap)
{
	const int table = lua_gettop(L);
	if (!lua_istable(L, table)) {
		return false;
	}
	lua_pushnil(L);
	while (lua_next(L, table) != 0) {
		if (!lua_isnumber(L, -1) || !lua_isnumber(L, -2)) {
			return false;
		}
		const int key = (int)lua_tonumber(L, -2);
		const int value = (int)lua_tonumber(L, -1);
		intMap[key] = value;
		lua_pop(L, 1); // pop the value, leave the key for the next iteration
	}
	
	lua_pop(L, 1); // pop the table
	
	return true;
}


bool CIconLayoutHandler::GetLuaIntList(lua_State* L, vector<int>& intList)
{
	const int table = lua_gettop(L);
	if (!lua_istable(L, table)) {
		return false;
	}
	lua_pushnil(L);
	while (lua_next(L, table) != 0) {
		if (!lua_isnumber(L, -1)) { // the value  (key is at -2)
			return false;
		}
		const int value = (int)lua_tonumber(L, -1);
		intList.push_back(value);
		lua_pop(L, 1); // pop the value, leave the key for the next iteration
	}
	
	lua_pop(L, 1); // pop the table
	
	return true;
}


bool CIconLayoutHandler::GetLuaReStringList(lua_State* L,
                                            vector<ReStringPair>& reStringList)
{
	const int table = lua_gettop(L);
	if (!lua_istable(L, table)) {
		return false;
	}

	lua_pushnil(L);
	while (lua_next(L, table) != 0) {
		if (!lua_isnumber(L, -2) || !lua_isstring(L, -1)) {
			return false;
		}
		ReStringPair rp;
		rp.cmdIndex = (int)lua_tonumber(L, -2);
		rp.texture = lua_tostring(L, -1);
		reStringList.push_back(rp);
		lua_pop(L, 1); // pop the value, leave the key for the next iteration
	}
	
	lua_pop(L, 1); // pop the table
	
	return true;
}


bool CIconLayoutHandler::GetLuaReParamsList(lua_State* L,
                                            vector<ReParamsPair>& reParamsCmds)
{
	const int table = lua_gettop(L);
	if (!lua_istable(L, table)) {
		return false;
	}

	lua_pushnil(L);
	while (lua_next(L, table) != 0) {
		if (!lua_isnumber(L, -2) || !lua_istable(L, -1)) {
			return false;
		}
		ReParamsPair paramsPair;
		paramsPair.cmdIndex = (int)lua_tonumber(L, -2);
		const int paramTable = lua_gettop(L);
		lua_pushnil(L);
		while (lua_next(L, paramTable) != 0) {
			if (!lua_isnumber(L, -2) || !lua_isstring(L, -1)) {
				return false;
			}
			const int paramIndex = (int)lua_tonumber(L, -2);
			const string paramValue = lua_tostring(L, -1);
			paramsPair.params[paramIndex] = paramValue;
			lua_pop(L, 1); // pop the value, leave the key for the next iteration
		}
		reParamsCmds.push_back(paramsPair);
		
		lua_pop(L, 1); // pop the value, leave the key for the next iteration
	}
	
	lua_pop(L, 1); // pop the table
	
	return true;
}


bool CIconLayoutHandler::GetLuaCmdDescList(lua_State* L,
                                           vector<CommandDescription>& cmdDescs)
{
	const int table = lua_gettop(L);
	if (!lua_istable(L, table)) {
		return false;
	}
	lua_pushnil(L);
	while (lua_next(L, table) != 0) {
		if (!lua_istable(L, -1)) {
			return false;
		}
		CommandDescription cd;
		cd.id   = CMD_INTERNAL;
		cd.type = CMDTYPE_CUSTOM;
		const int cmdDescTable = lua_gettop(L);
		lua_pushnil(L);
		while (lua_next(L, cmdDescTable) != 0) {
			if (lua_isstring(L, -2) && lua_isstring(L, -1)) {
				const string key = StringToLower(lua_tostring(L, -2));
				const string value = lua_tostring(L, -1);
				if (key == "name") {
					cd.name = value;
				} else if (key == "action") {
					cd.action = value;
				} else if (key == "iconname") {
					cd.iconname = value;
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
			lua_pop(L, 1); // pop the value
		}
		lua_pop(L, 1); // pop the value

		cmdDescs.push_back(cd);
	}

	lua_pop(L, 1); // pop the table

	return true;
}


/******************************************************************************/
/******************************************************************************/
//
// Lua Callbacks
//

static int SendCommands(lua_State* L)
{
	if (guihandler == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_istable(L, -1)) {
		lua_pushstring(L, "Incorrect arguments to SendCommands()");
		lua_error(L);
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


static int GetFPS(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetFPS() takes no arguments");
		lua_error(L);
	}
	if (game) {
		lua_pushnumber(L, game->fps);
	} else {
		lua_pushnumber(L, 0);
	}
	return 1;
}


static int GetInCommand(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetInCommand() takes no arguments");
		lua_error(L);
	}
	if (guihandler != NULL) {
		const int inCommand = guihandler->inCommand;
		lua_pushnumber(L, inCommand);
		if ((inCommand < 0) && (inCommand >= guihandler->commands.size())) {
			return 1;
		} else {
			lua_pushnumber(L, guihandler->commands[inCommand].id);
			lua_pushnumber(L, guihandler->commands[inCommand].type);
			lua_pushstring(L, guihandler->commands[inCommand].name.c_str());
			return 4;
		}
	}
	return 0;
}


static int GetConfigInt(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || (args > 2) || !lua_isstring(L, 1) ||
	    ((args == 2) && !lua_isnumber(L, 2))) {
		lua_pushstring(L, "Incorrect arguments to GetConfigInt()");
		lua_error(L);
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


static int SetConfigInt(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isstring(L, 1) || !lua_isnumber(L, 2)) {
		lua_pushstring(L, "Incorrect arguments to SetConfigInt()");
		lua_error(L);
	}
	const string name = lua_tostring(L, 1);
	const int value = (int)lua_tonumber(L, 2);
	configHandler.SetInt(name, value);
	return 0;
}


static int GetConfigString(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || (args > 2) || !lua_isstring(L, 1) ||
	    ((args == 2) && !lua_isstring(L, 2))) {
		lua_pushstring(L, "Incorrect arguments to GetConfigString()");
		lua_error(L);
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


static int SetConfigString(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isstring(L, 1) || !lua_isstring(L, 2)) {
		lua_pushstring(L, "Incorrect arguments to SetConfigString()");
		lua_error(L);
	}
	const string name = lua_tostring(L, 1);
	const string value = lua_tostring(L, 2);
	configHandler.SetString(name, value);
	return 0;
}


enum UnitExtraParam {
	UnitDefId,
	UnitGroupId,
	UnitTeamId
};

static void PackUnitsSet(lua_State* L, const set<CUnit*>& unitSet,
                         UnitExtraParam extraParam)
{
	map<int, vector<CUnit*> > unitDefMap;
	set<CUnit*>::const_iterator uit;
	for (uit = unitSet.begin(); uit != unitSet.end(); ++uit) {
		CUnit* unit = *uit;
		if (unit->unitDef == NULL) {
			continue;
		}
		unitDefMap[unit->unitDef->id].push_back(unit);
	}

	lua_newtable(L);
	map<int, vector<CUnit*> >::const_iterator mit;
	for (mit = unitDefMap.begin(); mit != unitDefMap.end(); mit++) {
		lua_pushnumber(L, mit->first); // push the UnitDef index
		lua_newtable(L);
		const vector<CUnit*>& v = mit->second;
		for (int i = 0; i < (int)v.size(); i++) {
			CUnit* unit = v[i];
			switch (extraParam) {
				case UnitGroupId: {
					const int gID = (unit->group == NULL) ? -1 : unit->group->id;
					lua_pushnumber(L, unit->id);
					lua_pushnumber(L, gID);
					break;
				}
				case UnitTeamId: {
					lua_pushnumber(L, unit->id);
					lua_pushnumber(L, unit->team);
					break;
				}
				default: { // UnitDefId
					lua_pushnumber(L, unit->id);
					lua_pushnumber(L, unit->unitDef->id); // convenience
					break;
				}
			}
			lua_rawset(L, -3);
		}
		lua_pushliteral(L, "n");
		lua_pushnumber(L, v.size());
		lua_rawset(L, -3);
		
		lua_rawset(L, -3);
	}

	lua_pushliteral(L, "n");
	lua_pushnumber(L, unitDefMap.size());
	lua_rawset(L, -3);
}


static int GetSelectedUnits(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetSelectedUnits() takes no arguments");
		lua_error(L);
	}
	const set<CUnit*>& selUnits = selectedUnits.selectedUnits;
	PackUnitsSet(L, selectedUnits.selectedUnits, UnitGroupId);
	lua_pushnumber(L, selectedUnits.selectedGroup);
	return 2;
}


static int GetGroupList(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetGroupList() takes no arguments");
		lua_error(L);
	}
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


static int GetGroupUnits(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to GetGroupUnits(groupID)");
		lua_error(L);
	}
	const int groupID = (int)lua_tonumber(L, 1);
	lua_pop(L, 1);
	const vector<CGroup*>& groups = grouphandler->groups;
	if ((groupID < 0) || (groupID >= groups.size()) ||
	    (groups[groupID] == NULL)) {
		return 0; // nils
	}
	PackUnitsSet(L, groups[groupID]->units, UnitDefId);
	return 1;
}


static int GetMyTeamUnits(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetMyTeamUnits() takes no arguments");
		lua_error(L);
	}
	PackUnitsSet(L, gs->Team(gu->myTeam)->units, UnitDefId);
	return 1;
}


static int GetAlliedUnits(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetAlliedUnits() takes no arguments");
		lua_error(L);
	}
	int unitCount = 0;

	lua_newtable(L);
	int count = 0;
	for (int t = 0; t < MAX_TEAMS; t++) {
		if (gs->AlliedTeams(t, gu->myTeam)) {
			lua_pushnumber(L, t);
			PackUnitsSet(L, gs->Team(t)->units, UnitTeamId);
			lua_rawset(L, -3);
			count++;
			unitCount += gs->Team(t)->units.size();
		}
	}
	lua_pushliteral(L, "n");
	lua_pushnumber(L, count);
	lua_rawset(L, -3);

	lua_pushnumber(L, unitCount);

	return 2;
}


static int GetBuildQueue(lua_State* L, bool canBuild)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, -1)) {
		lua_pushstring(L, "Incorrect arguments to GetBuildQueue(unitID)");
		lua_error(L);
	}
	const int unitID = (int)lua_tonumber(L, 1);
	lua_pop(L, 1);
	if ((unitID < 0) || (unitID >= MAX_UNITS) || (uh->units[unitID] == NULL)) {
		char buf[128];
		SNPRINTF(buf, sizeof(buf), "GetBuildQueue(%i) bad unitID", unitID);
		lua_pushstring(L, buf);
		return 1;
	}
	const CUnit* unit = uh->units[unitID];
	if (unit == NULL) { return 0; }
	const CCommandAI* commandAI = unit->commandAI;
	if (commandAI == NULL) { return 0; }

	lua_newtable(L);
	
	int entry = 0;
	int currentType = -1;
	int currentCount = 0;
	const deque<Command>& commandQue = commandAI->commandQue;
	deque<Command>::const_iterator it;
	for (it = commandQue.begin(); it != commandQue.end(); it++) {
		if (it->id < 0) { // a build command
			const int unitDefID = -(it->id);

			if (canBuild) {
				// skip build orders that this unit can not start
				const UnitDef* order_ud = unitDefHandler->GetUnitByID(unitDefID);
				const UnitDef* builder_ud = unit->unitDef;
				if ((order_ud == NULL) || (builder_ud == NULL)) {
					continue; // something is wrong, bail
				}
				const string& orderName = StringToLower(order_ud->name);
				const std::map<int, std::string>& bOpts = builder_ud->buildOptions;
				std::map<int, std::string>::const_iterator it;
				for (it = bOpts.begin(); it != bOpts.end(); ++it) {
					if (it->second == orderName) {
						break;
					}
				}
				if (it == bOpts.end()) {
					continue; // didn't find a matching entry
				}
			}

			if (currentType == unitDefID) {
				currentCount++;
			} else if (currentType == -1) {
				currentType = unitDefID;
				currentCount = 1;
			} else {
				entry++;
				lua_pushnumber(L, entry);
				lua_newtable(L);
				lua_pushnumber(L, currentType);
				lua_pushnumber(L, currentCount);
				lua_rawset(L, -3);
				lua_rawset(L, -3);
				currentType = unitDefID;
				currentCount = 1;
			}
		}
	}
	
	if (currentCount > 0) {
		entry++;
		lua_pushnumber(L, entry);
		lua_newtable(L);
		lua_pushnumber(L, currentType);
		lua_pushnumber(L, currentCount);
		lua_rawset(L, -3);
		lua_rawset(L, -3);
	}
	
	lua_pushnumber(L, entry);

	return 2;
}


static int GetFullBuildQueue(lua_State* L)
{
	return GetBuildQueue(L, false);
}


static int GetRealBuildQueue(lua_State* L)
{
	return GetBuildQueue(L, true);
}


static int GetAllyteamList(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetAllyteamList() takes no arguments");
		lua_error(L);
	}
	lua_newtable(L);
	for (int at = 0; at < gs->activeAllyTeams; at++) {
		lua_pushnumber(L, at);
		lua_newtable(L);
		for (int team = 0; team < gs->activeTeams; team++) {
			if ((gs->Team(team) != NULL) && gs->Ally(at, gs->AllyTeam(team))) {
				lua_pushnumber(L, team);
				lua_newtable(L);
				for (int p = 0; p < gs->activePlayers; p++) {
					if ((gs->players[p] != NULL) &&
					    (gs->players[p]->active) &&
					    (gs->players[p]->team == team)) {
						lua_pushnumber(L, p);
						bool isAI = false;
						if ((globalAI != NULL) && (globalAI->ais[team] != NULL)) {
							isAI = true;
						}
						lua_pushboolean(L, isAI);
						lua_rawset(L, -3);
					}
				}
				lua_rawset(L, -3); // teamID and player table
			}
		}
		lua_rawset(L, -3); // allyteamID and teamID table
	}
	
	return 1;
}


static int GetTeamInfo(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, -1)) {
		lua_pushstring(L, "Incorrect arguments to GetTeamInfo(teamID)");
		lua_error(L);
	}

	const int teamID = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);
	if ((teamID < 0) || (teamID >= MAX_PLAYERS)) {
		return 0;
	}

	const CTeam* team = gs->Team(teamID);
	if (team == NULL) {
		return 0;
	}
	
	lua_pushnumber(L, team->teamNum);
	lua_pushnumber(L, team->leader);
	lua_pushboolean(L, team->active);
	lua_pushboolean(L, team->isDead);
	lua_pushstring(L, team->side.c_str());
	lua_pushnumber(L, team->color[0]);
	lua_pushnumber(L, team->color[1]);
	lua_pushnumber(L, team->color[2]);
	lua_pushnumber(L, team->color[3]);
	
	return 9;
}


static int GetPlayerInfo(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, -1)) {
		lua_pushstring(L, "Incorrect arguments to GetPlayerInfo(playerID)");
		lua_error(L);
	}

	const int playerID = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);
	if ((playerID < 0) || (playerID >= MAX_PLAYERS)) {
		return 0;
	}

	const CPlayer* player = gs->players[playerID];
	if (player == NULL) {
		return 0;
	}	

	lua_pushstring(L, player->playerName.c_str());
	lua_pushboolean(L, player->active);
	lua_pushboolean(L, player->spectator);
	lua_pushnumber(L, player->team);
	lua_pushnumber(L, gs->AllyTeam(player->team));
	lua_pushnumber(L, player->ping);
	lua_pushnumber(L, player->cpuUsage);

	return 7;
}


static int GetMyPlayerID(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetMyPlayerID() takes no arguments");
		lua_error(L);
	}
	lua_pushnumber(L, gu->myPlayerNum);
	return 1;
}


static int AreTeamsAllied(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isnumber(L, -1) || !lua_isnumber(L, -2)) {
		lua_pushstring(L, "Incorrect arguments to AreTeamsAllied(team1, team2)");
		lua_error(L);
	}
	const int team1 = (int)lua_tonumber(L, -1);
	const int team2 = (int)lua_tonumber(L, -2);
	if ((team1 < 0) || (team1 >= MAX_TEAMS) ||
	    (team2 < 0) || (team2 >= MAX_TEAMS)) {
		return 0;
	}
	lua_pushboolean(L, gs->AlliedTeams(team1, team2));
	return 1;
}


static int ArePlayersAllied(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isnumber(L, -1) || !lua_isnumber(L, -2)) {
		lua_pushstring(L, "Incorrect arguments to ArePlayersAllied(p1, p2)");
		lua_error(L);
	}
	const int player1 = (int)lua_tonumber(L, -1);
	const int player2 = (int)lua_tonumber(L, -2);
	if ((player1 < 0) || (player1 >= MAX_PLAYERS) ||
	    (player2 < 0) || (player2 >= MAX_PLAYERS)) {
		return 0;
	}
	const CPlayer* p1 = gs->players[player1];
	const CPlayer* p2 = gs->players[player2];
	if ((p1 == NULL) || (p2 == NULL)) {
		return 0;
	}
	lua_pushboolean(L, gs->AlliedTeams(p1->team, p2->team));
	return 1;
}


/******************************************************************************/
