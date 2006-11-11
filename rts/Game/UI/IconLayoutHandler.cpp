#include "StdAfx.h"
// IconLayoutHandler.cpp: implementation of the CIconLayoutHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "IconLayoutHandler.h"
#include <cctype>
#include <SDL_keysym.h>
#include <SDL_mouse.h>
extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}
#include "LuaState.h"
#include "CursorIcons.h"
#include "ExternalAI/GlobalAIHandler.h"
#include "ExternalAI/Group.h"
#include "ExternalAI/GroupHandler.h"
#include "Game/command.h"
#include "Game/CameraController.h"
#include "Game/Game.h"
#include "Game/GameHelper.h"
#include "Game/SelectedUnits.h"
#include "Game/Team.h"
#include "Game/UI/GuiHandler.h"
#include "Game/UI/MouseHandler.h"
#include "Map/Ground.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/glFont.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Weapons/Weapon.h"
#include "System/LogOutput.h"
#include "System/Net.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Platform/ConfigHandler.h"
#include "System/Sound.h"

extern Uint8 *keys;


/******************************************************************************/

// LUA C functions  (placed in the "Spring" table)

static int LoadTextVFS(lua_State* L);

static int GetConfigInt(lua_State* L);
static int SetConfigInt(lua_State* L);
static int GetConfigString(lua_State* L);
static int SetConfigString(lua_State* L);

static int GetFPS(lua_State* L);
static int GetGameSeconds(lua_State* L);
static int GetInCommand(lua_State* L);
static int GetMouseState(lua_State* L);

static int SendCommands(lua_State* L);
static int GiveOrder(lua_State* L);

static int GetGroupList(lua_State* L);
static int GetSelectedGroup(lua_State* L);

static int GetSelectedUnits(lua_State* L);
static int GetGroupUnits(lua_State* L);
static int GetMyTeamUnits(lua_State* L);
static int GetAlliedUnits(lua_State* L);

static int GetUnitDefID(lua_State* L);
static int GetUnitTeam(lua_State* L);
static int GetUnitAllyTeam(lua_State* L);
static int GetUnitHealth(lua_State* L);
static int GetUnitStates(lua_State* L);
static int GetUnitStockpile(lua_State* L);
static int GetUnitPosition(lua_State* L);
static int GetUnitHeading(lua_State* L);
static int GetUnitBuildFacing(lua_State* L);

static int GetCommandQueue(lua_State* L);
static int GetFullBuildQueue(lua_State* L);
static int GetRealBuildQueue(lua_State* L);

static int GetAllyteamList(lua_State* L);
static int GetTeamInfo(lua_State* L);
static int GetTeamResources(lua_State* L);
static int SetShareLevel(lua_State* L);

static int GetPlayerInfo(lua_State* L);
static int GetMyPlayerID(lua_State* L);

static int AreTeamsAllied(lua_State* L);
static int ArePlayersAllied(lua_State* L);

static int GetCameraState(lua_State* L);
static int SetCameraState(lua_State* L);

static int GetMapHeight(lua_State* L);
static int TestBuildOrder(lua_State* L);

static int AddMapIcon(lua_State* L);
static int AddMapText(lua_State* L);
static int AddMapUnit(lua_State* L);

static int DrawState(lua_State* L);
static int DrawShape(lua_State* L);
static int DrawText(lua_State* L);
static int DrawTranslate(lua_State* L);
static int DrawScale(lua_State* L);
static int DrawRotate(lua_State* L);
static int DrawPushMatrix(lua_State* L);
static int DrawPopMatrix(lua_State* L);
static int DrawListCreate(lua_State* L);
static int DrawListRun(lua_State* L);
static int DrawListDelete(lua_State* L);

static int PlaySoundFile(lua_State* L);


/******************************************************************************/

// Local Variables

static bool drawingEnabled = false;
static bool screenTransform = false;

static int matrixDepth = 0;
static const int maxMatrixDepth = 16;

static vector<unsigned int> displayLists;


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
	for (int i = 0; i < (int)displayLists.size(); i++) {
		glDeleteLists(displayLists[i], 1);
	}
}


string CIconLayoutHandler::LoadFile(const string& filename)
{
	string code;

	CFileHandler fh(filename); 

	if (fh.FileExists()) {
		while (fh.Peek() != EOF) {
			int c;
			fh.Read(&c, 1);
			code += (char)c;
		}
	}
	
	return code;
}


bool CIconLayoutHandler::LoadCFunctions(lua_State* L)
{
	lua_newtable(L);

#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)
	
	REGISTER_LUA_CFUNC(LoadTextVFS);
	REGISTER_LUA_CFUNC(SendCommands);
	REGISTER_LUA_CFUNC(GetFPS);
	REGISTER_LUA_CFUNC(GetGameSeconds);
	REGISTER_LUA_CFUNC(GetInCommand);
	REGISTER_LUA_CFUNC(GetMouseState);
	REGISTER_LUA_CFUNC(GetConfigInt);
	REGISTER_LUA_CFUNC(SetConfigInt);
	REGISTER_LUA_CFUNC(GetConfigString);
	REGISTER_LUA_CFUNC(SetConfigString);
	REGISTER_LUA_CFUNC(GetGroupList);
	REGISTER_LUA_CFUNC(GetSelectedGroup);
	REGISTER_LUA_CFUNC(GetSelectedUnits);
	REGISTER_LUA_CFUNC(GetGroupUnits);
	REGISTER_LUA_CFUNC(GetMyTeamUnits);
	REGISTER_LUA_CFUNC(GetAlliedUnits);
	REGISTER_LUA_CFUNC(GetUnitDefID);
	REGISTER_LUA_CFUNC(GetUnitTeam);
	REGISTER_LUA_CFUNC(GetUnitAllyTeam);
	REGISTER_LUA_CFUNC(GetUnitHealth);
	REGISTER_LUA_CFUNC(GetUnitStates);
	REGISTER_LUA_CFUNC(GetUnitStockpile);
	REGISTER_LUA_CFUNC(GetUnitPosition);
	REGISTER_LUA_CFUNC(GetUnitHeading);
	REGISTER_LUA_CFUNC(GetUnitBuildFacing);
	REGISTER_LUA_CFUNC(GetCommandQueue);
	REGISTER_LUA_CFUNC(GetFullBuildQueue);
	REGISTER_LUA_CFUNC(GetRealBuildQueue);
	REGISTER_LUA_CFUNC(GetAllyteamList);
	REGISTER_LUA_CFUNC(GetTeamInfo);
	REGISTER_LUA_CFUNC(GetTeamResources);
	REGISTER_LUA_CFUNC(SetShareLevel);
	REGISTER_LUA_CFUNC(GetPlayerInfo);
	REGISTER_LUA_CFUNC(GetMyPlayerID);
	REGISTER_LUA_CFUNC(AreTeamsAllied);
	REGISTER_LUA_CFUNC(ArePlayersAllied);
	REGISTER_LUA_CFUNC(GetCameraState);
	REGISTER_LUA_CFUNC(SetCameraState);
	REGISTER_LUA_CFUNC(GiveOrder);
	REGISTER_LUA_CFUNC(GetMapHeight);
	REGISTER_LUA_CFUNC(TestBuildOrder);
	REGISTER_LUA_CFUNC(AddMapIcon);
	REGISTER_LUA_CFUNC(AddMapText);
	REGISTER_LUA_CFUNC(AddMapUnit);
	REGISTER_LUA_CFUNC(DrawState);
	REGISTER_LUA_CFUNC(DrawShape);
	REGISTER_LUA_CFUNC(DrawText);
	REGISTER_LUA_CFUNC(DrawListCreate);
	REGISTER_LUA_CFUNC(DrawListRun);
	REGISTER_LUA_CFUNC(DrawListDelete);
	REGISTER_LUA_CFUNC(DrawTranslate);
	REGISTER_LUA_CFUNC(DrawScale);
	REGISTER_LUA_CFUNC(DrawRotate);
	REGISTER_LUA_CFUNC(DrawPushMatrix);
	REGISTER_LUA_CFUNC(DrawPopMatrix);
	REGISTER_LUA_CFUNC(PlaySoundFile);

	lua_setglobal(L, "Spring");
	
	return true;
}


bool CIconLayoutHandler::LoadCode(lua_State* L,
                                  const string& code, const string& debug)
{
  lua_pop(L, lua_gettop(L));

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
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return true; // the call was not implemented
	}

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


bool CIconLayoutHandler::UpdateLayout(bool& forceLayout,
                                      bool commandsChanged, int activePage)
{
	forceLayout = false;

	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "UpdateLayout");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		forceLayout = false;
		return true; // the call was not implemented, no forced updates
	}

	lua_pushboolean(L, commandsChanged);
	lua_pushnumber(L,  activePage);
	lua_pushboolean(L, keys[SDLK_LALT]);
	lua_pushboolean(L, keys[SDLK_LCTRL]);
	lua_pushboolean(L, keys[SDLK_LMETA]);
	lua_pushboolean(L, keys[SDLK_LSHIFT]);
	const int error = lua_pcall(L, 6, 1, 0);
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


bool CIconLayoutHandler::CommandNotify(const Command& cmd)
{
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));
	
	lua_getglobal(L, "CommandNotify");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return true; // the call was not implemented
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
	lua_pushstring(L, "alt");
	lua_pushboolean(L, cmd.options & ALT_KEY);
	lua_rawset(L, -3);
	lua_pushstring(L, "ctrl");
	lua_pushboolean(L, cmd.options & CONTROL_KEY);
	lua_rawset(L, -3);
	lua_pushstring(L, "shift");
	lua_pushboolean(L, cmd.options & SHIFT_KEY);
	lua_rawset(L, -3);
	lua_pushstring(L, "right");
	lua_pushboolean(L, cmd.options & RIGHT_MOUSE_KEY);
	lua_rawset(L, -3);
	
	// call the function
	const int error = lua_pcall(L, 3, 1, 0);
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n", error,
		                "Call_CommandNotify", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}

	// get the results
	const int args = lua_gettop(L);
	if ((args != 1) || !lua_isboolean(L, -1)) {
		logOutput.Print("CommandNotify() bad return value (%i)\n", args);
		lua_pop(L, args);
		return true;
	}
	
	return !!lua_toboolean(L, -1);
}


bool CIconLayoutHandler::UnitCreated(CUnit* unit)
{
	if (unit->allyteam != gu->myAllyTeam) {
		return false;
	}
	
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "UnitCreated");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return true; // the call was not implemented
	}

	lua_pushnumber(L, unit->id);	
	lua_pushnumber(L, unit->unitDef->id);	

	// call the routine
	const int error = lua_pcall(L, 2, 0, 0);
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n", error,
		                "Call_UnitCreated", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}

	return true;	
}


bool CIconLayoutHandler::UnitReady(CUnit* unit, CUnit* builder)
{
	if (unit->allyteam != gu->myAllyTeam) {
		return false;
	}
	
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "UnitReady");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return true; // the call was not implemented
	}

	lua_pushnumber(L, unit->id);	
	lua_pushnumber(L, unit->unitDef->id);	
	lua_pushnumber(L, builder->id);	
	lua_pushnumber(L, builder->unitDef->id);	

	// call the routine
	const int error = lua_pcall(L, 4, 0, 0);
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n", error,
		                "Call_UnitReady", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}

	return true;	
}


bool CIconLayoutHandler::UnitDestroyed(CUnit* victim, CUnit* attacker)
{
	if (victim->allyteam != gu->myAllyTeam) {
		return false;
	}
	
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "UnitDestroyed");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return true; // the call was not implemented
	}

	lua_pushnumber(L, victim->id);	
	lua_pushnumber(L, victim->unitDef->id);	

	// call the routine
	const int error = lua_pcall(L, 2, 0, 0);
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n", error,
		                "Call_UnitDestroyed", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}

	return true;
}


bool CIconLayoutHandler::DrawMapItems()
{
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));
	
	lua_getglobal(L, "DrawMapItems");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return true; // the call was not implemented
	}

	drawingEnabled = true;

	lua_pushnumber(L, gu->screenx);	// FIXME -- better input
	lua_pushnumber(L, gu->screeny);	
	
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glPushMatrix();
	matrixDepth = 0;

	// call the routine
	const int error = lua_pcall(L, 2, 0, 0);
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n", error,
		                "Call_DrawMapItems", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}
	
	if (matrixDepth != 0) {
		logOutput.Print("DrawMapItems: matrix mismatch");
	}
	while (matrixDepth > 0) {
		glPopMatrix();
		matrixDepth--;
	}
	glPopMatrix();
	
	drawingEnabled = false;
	
	return true;
}


bool CIconLayoutHandler::DrawScreenItems()
{
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "DrawScreenItems");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return true; // the call was not implemented
	}

	drawingEnabled = true;
	screenTransform = true;

	lua_pushnumber(L, gu->screenx);	
	lua_pushnumber(L, gu->screeny);	

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glPushMatrix();
	matrixDepth = 0;
	
	// setup the screen transformation
	glScalef(1.0f / float(gu->screenx), 1.0f / float(gu->screeny), 1.0f);
	
	// call the routine
	const int error = lua_pcall(L, 2, 0, 0);
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n", error,
		                "Call_DrawScreenItems", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}
	
	if (matrixDepth != 0) {
		logOutput.Print("DrawScreenItems: matrix mismatch");
	}
	while (matrixDepth > 0) {
		glPopMatrix();
		matrixDepth--;
	}
	glPopMatrix();
	
	screenTransform = false;
	drawingEnabled = false;
	
	return true;
}


bool CIconLayoutHandler::MouseMove(int x, int y, int dx, int dy, int button)
{
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "MouseMove");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return false; // the call was not implemented, do not take the event
	}

	lua_pushnumber(L, x);
	lua_pushnumber(L, gu->screeny - y - 1);
	lua_pushnumber(L, dx);
	lua_pushnumber(L, -dy);
	lua_pushnumber(L, button);

	// call the function
	const int error = lua_pcall(L, 5, 1, 0);
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n", error,
		                "Call_MouseMove", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}

	const int args = lua_gettop(L);
	if ((args == 1) && lua_isboolean(L, -1)) {
		return !!lua_toboolean(L, -1);
	}

	return false;
}


bool CIconLayoutHandler::MousePress(int x, int y, int button)
{
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "MousePress");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return false; // the call was not implemented, do not take the event
	}

	lua_pushnumber(L, x);
	lua_pushnumber(L, gu->screeny - y - 1);
	lua_pushnumber(L, button);

	// call the function
	const int error = lua_pcall(L, 3, 1, 0);
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n", error,
		                "Call_MousePress", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}

	const int args = lua_gettop(L);
	if ((args == 1) && lua_isboolean(L, -1)) {
		return !!lua_toboolean(L, -1);
	}

	return false;
}


bool CIconLayoutHandler::MouseRelease(int x, int y, int button)
{
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "MouseRelease");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return false; // the call was not implemented, do not take the event
	}

	lua_pushnumber(L, x);
	lua_pushnumber(L, gu->screeny - y - 1);
	lua_pushnumber(L, button);

	// call the function
	const int error = lua_pcall(L, 3, 1, 0);
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n", error,
		                "Call_MouseRelease", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}

	const int args = lua_gettop(L);
	if ((args == 1) && lua_isboolean(L, -1)) {
		return !!lua_toboolean(L, -1);
	}

	return false;
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
	lua_pop(L, lua_gettop(L));

	customCmds.clear();
	removeCmds.clear();
	reTextureCmds.clear();
	reNamedCmds.clear();
	reTooltipCmds.clear();
	onlyTextureCmds.clear();
	iconList.clear();
	menuName = "";

	lua_getglobal(L, "LayoutIcons");
	if (!lua_isfunction(L, -1)) {
		logOutput.Print("Error: missing LayoutIcons() lua function\n");
		lua_pop(L, lua_gettop(L));
		return false;
	}
	
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
		                "Call_LayoutIcons", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}

	// get the results
	const int args = lua_gettop(L);
	
	if ((args == 1) && (lua_isstring(L, -1)) &&
	    (string(lua_tostring(L, -1)) == "disabled")) {
		return false; // no warnings
	}
	else if (args != 11) {
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

static int LoadTextVFS(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isstring(L, -1)) {
		lua_pushstring(L, "Incorrect arguments to LoadTextVFS(filename)");
		lua_error(L);
	}
	const string& filename = lua_tostring(L, -1);
	CFileHandler fh(filename);
	if (!fh.FileExists()) {
		return 0;
	}
	
	string buf;
	while (fh.Peek() != EOF) {
		int c;
		fh.Read(&c, 1);
		buf += (char)c;
	}
	lua_pushstring(L, buf.c_str());

	return 1;
}


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


static int GetGameSeconds(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetGameSeconds() takes no arguments");
		lua_error(L);
	}
	const float seconds = ((float)gs->frameNum / 30.0f);
	lua_pushnumber(L, seconds);
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


static int GetMouseState(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetMouseState() takes no arguments");
		lua_error(L);
	}
	lua_pushnumber(L, mouse->lastx);
	lua_pushnumber(L, gu->screeny - mouse->lasty - 1);
	lua_pushboolean(L, mouse->buttons[SDL_BUTTON_LEFT].pressed);
	lua_pushboolean(L, mouse->buttons[SDL_BUTTON_MIDDLE].pressed);
	lua_pushboolean(L, mouse->buttons[SDL_BUTTON_RIGHT].pressed);
	return 5;
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


/******************************************************************************/

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


static int GetSelectedGroup(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetSelectedGroup() takes no arguments");
		lua_error(L);
	}
	lua_pushnumber(L, selectedUnits.selectedGroup);
	return 1;
}


/******************************************************************************/

enum UnitSetFormat {
	SortedUnitSet,
	UnsortedUnitSet
};

enum UnitExtraParam {
	UnitDefId,
	UnitGroupId,
	UnitTeamId
};


static void PackUnitsSet(lua_State* L, const set<CUnit*>& unitSet,
                         UnitExtraParam extraParam, UnitSetFormat format)
{
	set<CUnit*>::const_iterator uit;

	// UnsortedUnitSet
	if (format == UnsortedUnitSet) {
		lua_newtable(L);
		int i = 0;
		for (uit = unitSet.begin(); uit != unitSet.end(); ++uit) {
			i++;
			lua_pushnumber(L, i);
			lua_pushnumber(L, (*uit)->id);
			lua_rawset(L, -3);
		}
		lua_pushstring(L, "n");
		lua_pushnumber(L, unitSet.size());
		lua_rawset(L, -3);
		return;
	}

	// SortedUnitSet
	map<int, vector<CUnit*> > unitDefMap;
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
					const int groupID = (unit->group == NULL) ? -1 : unit->group->id;
					lua_pushnumber(L, unit->id);
					lua_pushnumber(L, groupID);
					break;
				}
				case UnitTeamId: {
					lua_pushnumber(L, unit->id);
					lua_pushnumber(L, unit->team);
					break;
				}
				default: { // UnitDefId
					lua_pushnumber(L, unit->id);
					lua_pushnumber(L, unit->unitDef->id);
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
	
	return;
}


static int GetSelectedUnits(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args > 0) && !lua_isstring(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to GetSelectedUnits([\"raw\"])");
		lua_error(L);
	}
	UnitSetFormat format = SortedUnitSet;
	if ((args >= 1) && (string(lua_tostring(L, 1)) == "raw")) {
		format = UnsortedUnitSet;
	}
	PackUnitsSet(L, selectedUnits.selectedUnits, UnitGroupId, format);
	return 1;
}


static int GetGroupUnits(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1) ||
	    ((args >= 2) && !lua_isstring(L, 2))) {
		lua_pushstring(L,
			"Incorrect arguments to GetGroupUnits(groupID [, \"raw\"])");
		lua_error(L);
	}
	const int groupID = (int)lua_tonumber(L, 1);
	const vector<CGroup*>& groups = grouphandler->groups;
	if ((groupID < 0) || (groupID >= groups.size()) ||
	    (groups[groupID] == NULL)) {
		return 0; // nils
	}
	UnitSetFormat format = SortedUnitSet;
	if ((args >= 2) && (string(lua_tostring(L, 2)) == "raw")) {
		format = UnsortedUnitSet;
	}
	PackUnitsSet(L, groups[groupID]->units, UnitDefId, format);
	return 1;
}


static int GetMyTeamUnits(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args >= 1) && !lua_isstring(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to GetMyTeamUnits([\"raw\"])");
		lua_error(L);
	}
	UnitSetFormat format = SortedUnitSet;
	if ((args >= 1) && (string(lua_tostring(L, 1)) == "raw")) {
		format = UnsortedUnitSet;
	}
	PackUnitsSet(L, gs->Team(gu->myTeam)->units, UnitDefId, format);
	return 1;
}


static int GetAlliedUnits(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args >= 1) && !lua_isstring(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to GetAlliedUnits([\"raw\"])");
		lua_error(L);
	}
	UnitSetFormat format = SortedUnitSet;
	if ((args >= 1) && (string(lua_tostring(L, 1)) == "raw")) {
		format = UnsortedUnitSet;
	}

	int unitCount = 0;

	lua_newtable(L);
	int teamCount = 0;
	for (int t = 0; t < MAX_TEAMS; t++) {
		if (gs->AlliedTeams(t, gu->myTeam)) {
			lua_pushnumber(L, t);
			PackUnitsSet(L, gs->Team(t)->units, UnitTeamId, format);
			lua_rawset(L, -3);
			teamCount++;
			unitCount += gs->Team(t)->units.size();
		}
	}
	lua_pushliteral(L, "n");
	lua_pushnumber(L, teamCount);
	lua_rawset(L, -3);

	lua_pushnumber(L, unitCount);

	return 2;
}


/******************************************************************************/

static CUnit* AlliedUnit(lua_State* L, const char* caller)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1)) {
		char buf[256];
		SNPRINTF(buf, sizeof(buf), "Incorrect arguments to %s(unitID)", caller);
		lua_pushstring(L, buf);
		lua_error(L);
	}
	const int unitID = (int)lua_tonumber(L, -1);
	if ((unitID < 0) || (unitID >= MAX_UNITS)) {
		return NULL;
	}
	CUnit* unit = uh->units[unitID];
	if ((unit == NULL) || (unit->allyteam != gu->myAllyTeam)) {
		return NULL;
	}
	return unit;
}


static int GetUnitStates(lua_State* L)
{
	CUnit* unit = AlliedUnit(L, __FUNCTION__);
	if (unit == NULL) {
		return 0;
	}
	lua_newtable(L);
	lua_pushstring(L, "firestate");
	lua_pushnumber(L, unit->fireState);
	lua_rawset(L, -3);
	lua_pushstring(L, "movestate");
	lua_pushnumber(L, unit->moveState);
	lua_rawset(L, -3);
	lua_pushstring(L, "repeat");
	lua_pushboolean(L, unit->commandAI->repeatOrders);
	lua_rawset(L, -3);
	lua_pushstring(L, "cloak");
	lua_pushboolean(L, unit->wantCloak);
	lua_rawset(L, -3);
	lua_pushstring(L, "active");
	lua_pushboolean(L, unit->activated);
	lua_rawset(L, -3);
	lua_pushstring(L, "trajectory");
	lua_pushboolean(L, unit->useHighTrajectory);
	lua_rawset(L, -3);
/*	
	// FIXME -- finish these
	lua_pushstring(L, "autorepairlevel");
	lua_pushnumber(L, unit->);
	lua_rawset(L, -3);
	lua_pushstring(L, "loopbackattack");
	lua_pushnumber(L, unit->);
	lua_rawset(L, -3);
*/
	return 1;	
}


static int GetUnitStockpile(lua_State* L)
{
	CUnit* unit = AlliedUnit(L, __FUNCTION__);
	if (unit == NULL) {
		return 0;
	}
	if (unit->stockpileWeapon == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->stockpileWeapon->numStockpiled);
	lua_pushnumber(L, unit->stockpileWeapon->numStockpileQued);
	return 2;
}


static int GetUnitDefID(lua_State* L)
{
	CUnit* unit = AlliedUnit(L, __FUNCTION__);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->unitDef->id);
	return 1;
}


static int GetUnitTeam(lua_State* L)
{
	CUnit* unit = AlliedUnit(L, __FUNCTION__);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->team);
	return 1;
}


static int GetUnitAllyTeam(lua_State* L)
{
	CUnit* unit = AlliedUnit(L, __FUNCTION__);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->allyteam);
	return 1;
}


static int GetUnitHealth(lua_State* L)
{
	CUnit* unit = AlliedUnit(L, __FUNCTION__);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->health);
	lua_pushnumber(L, unit->maxHealth);
	lua_pushnumber(L, unit->paralyzeDamage);
	lua_pushnumber(L, unit->captureProgress);
	lua_pushnumber(L, unit->buildProgress);
	return 5;
}


static int GetUnitPosition(lua_State* L)
{
	CUnit* unit = AlliedUnit(L, __FUNCTION__);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->pos[0]);
	lua_pushnumber(L, unit->pos[1]);
	lua_pushnumber(L, unit->pos[2]);
	return 3;
}


static int GetUnitHeading(lua_State* L)
{
	CUnit* unit = AlliedUnit(L, __FUNCTION__);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->frontdir[0]);
	lua_pushnumber(L, unit->frontdir[1]);
	lua_pushnumber(L, unit->frontdir[2]);
	return 3;
}


static int GetUnitBuildFacing(lua_State* L)
{
	CUnit* unit = AlliedUnit(L, __FUNCTION__);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->buildFacing);
	return 1;
}


static int GetCommandQueue(lua_State* L)
{
	CUnit* unit = AlliedUnit(L, __FUNCTION__);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args >= 2) && !lua_isnumber(L, 2)) {
		lua_pushstring(L, "Incorrect arguments to GetCommandQueue(unitID [, count])");
		lua_error(L);
	}
	const CCommandAI* commandAI = unit->commandAI;
	if (commandAI == NULL) {
		return 0;
	}
 	const deque<Command>& commandQue = commandAI->commandQue;

	// get the desired number of commands to return
	int count = -1;
	if (args > 1) {
 		count = (int)lua_tonumber(L, 2);
	}
	if (count < 0) {
		count = (int)commandQue.size();
	}
 
	lua_newtable(L);
	
	int i = 0;
	deque<Command>::const_iterator it;
	for (it = commandQue.begin(); it != commandQue.end(); it++) {
		if (i >= count) {
			break;
		}
		
		const Command& cmd = *it;
		
		i++;
		lua_pushnumber(L, i);

		lua_newtable(L);

		lua_pushstring(L, "id");
		lua_pushnumber(L, cmd.id);
		lua_rawset(L, -3); // command id

		lua_pushstring(L, "params");
		lua_newtable(L);
		for (int p = 0; p < cmd.params.size(); p++) {
			lua_pushnumber(L, p + 1);
			lua_pushnumber(L, cmd.params[p]);
			lua_rawset(L, -3);
		}
		lua_rawset(L, -3); // params table

		lua_pushstring(L, "options");
		lua_newtable(L);
		lua_pushstring(L, "alt");
		lua_pushboolean(L, cmd.options & ALT_KEY);
		lua_rawset(L, -3);
		lua_pushstring(L, "ctrl");
		lua_pushboolean(L, cmd.options & CONTROL_KEY);
		lua_rawset(L, -3);
		lua_pushstring(L, "shift");
		lua_pushboolean(L, cmd.options & SHIFT_KEY);
		lua_rawset(L, -3);
		lua_pushstring(L, "right");
		lua_pushboolean(L, cmd.options & RIGHT_MOUSE_KEY);
		lua_rawset(L, -3);
		lua_pushstring(L, "internal");
		lua_pushboolean(L, cmd.options & INTERNAL_ORDER);
		lua_rawset(L, -3);
		lua_rawset(L, -3); // options table

		lua_rawset(L, -3); // push into the return table
	}

	lua_pushliteral(L, "n");
	lua_pushnumber(L, i);
	lua_rawset(L, -3);

	return 1;
}


static int GetBuildQueue(lua_State* L, bool canBuild, const char* caller)
{
	CUnit* unit = AlliedUnit(L, caller);
	if (unit == NULL) {
		return 0;
	}
	const CCommandAI* commandAI = unit->commandAI;
	if (commandAI == NULL) {
		return 0;
	}
	const deque<Command>& commandQue = commandAI->commandQue;

	lua_newtable(L);
	
	int entry = 0;
	int currentType = -1;
	int currentCount = 0;
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
	return GetBuildQueue(L, false, __FUNCTION__);
}


static int GetRealBuildQueue(lua_State* L)
{
	return GetBuildQueue(L, true, __FUNCTION__);
}


/******************************************************************************/

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
						lua_pushstring(L, gs->players[p]->playerName.c_str());
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
	if ((teamID < 0) || (teamID >= MAX_TEAMS)) {
		return 0;
	}

	const CTeam* team = gs->Team(teamID);
	if (team == NULL) {
		return 0;
	}

	bool isAiTeam = false;
	if ((globalAI != NULL) && (globalAI->ais[teamID] != NULL)) {
		isAiTeam = true;
	}
	
	lua_pushnumber(L, team->teamNum);
	lua_pushnumber(L, team->leader);
	lua_pushboolean(L, team->active);
	lua_pushboolean(L, team->isDead);
	lua_pushboolean(L, isAiTeam);
	lua_pushstring(L, team->side.c_str());
	lua_pushnumber(L, team->color[0]);
	lua_pushnumber(L, team->color[1]);
	lua_pushnumber(L, team->color[2]);
	lua_pushnumber(L, team->color[3]);
	
	return 10;
}


static int GetTeamResources(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, -1)) {
		lua_pushstring(L, "Incorrect arguments to GetTeamResources(teamID)");
		lua_error(L);
	}

	const int teamID = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);
	if ((teamID < 0) || (teamID >= MAX_TEAMS)) {
		return 0;
	}
	const CTeam* team = gs->Team(teamID);
	if ((team == NULL) || !gs->AlliedTeams(gu->myTeam, teamID)) {
		return 0;
	}
	
	lua_pushnumber(L, team->metal);
	lua_pushnumber(L, team->metalStorage);
	lua_pushnumber(L, team->prevMetalPull);
	lua_pushnumber(L, team->prevMetalIncome);
	lua_pushnumber(L, team->prevMetalExpense);
	lua_pushnumber(L, team->metalShare);
	lua_pushnumber(L, team->metalSent);
	lua_pushnumber(L, team->metalReceived);

	lua_pushnumber(L, team->energy);
	lua_pushnumber(L, team->energyStorage);
	lua_pushnumber(L, team->prevEnergyPull);
	lua_pushnumber(L, team->prevEnergyIncome);
	lua_pushnumber(L, team->prevEnergyExpense);
	lua_pushnumber(L, team->energyShare);
	lua_pushnumber(L, team->energySent);
	lua_pushnumber(L, team->energyReceived);
	
	return 16;
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


static int GetCameraState(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetCameraState() takes no arguments");
		lua_error(L);
	}

	const bool overview =
		(mouse->currentCamController == mouse->overviewController);

	lua_newtable(L);
	
	lua_pushstring(L, "mode");
	lua_pushnumber(L, overview ? -100 : mouse->currentCamControllerNum);
	lua_rawset(L, -3);
	
	vector<float> camState = mouse->currentCamController->GetState();
	for (int i = 0; i < (int)camState.size(); i++) {
		lua_pushnumber(L, i + 1);
		lua_pushnumber(L, camState[i]);
		lua_rawset(L, -3);
	}

	return 1;
}


static int SetCameraState(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_istable(L, 1) || !lua_isnumber(L, 2)) {
		lua_pushstring(L, "Incorrect arguments to SetCameraState(table, camTime)");
		lua_error(L);
	}
	
	const float camTime = (float)lua_tonumber(L, 2);
	lua_pop(L, 1);

	lua_pushstring(L, "mode");
	lua_gettable(L, -2);
	if (lua_isnumber(L, -1)) {
		const int camNum = (int)lua_tonumber(L, -1);
		if ((camNum == -100) ||
		    ((camNum >= 0) && (camNum < mouse->camControllers.size()) &&
		     mouse->camControllers[camNum]->enabled)) {
			const int oldNum = mouse->currentCamControllerNum;
			const float3 dummy = mouse->currentCamController->SwitchFrom();
			if (camNum >= 0) {
				mouse->currentCamControllerNum = camNum;
				mouse->currentCamController = mouse->camControllers[camNum];
			} else {
				mouse->currentCamController = mouse->overviewController;
			}
			const bool showMode = (oldNum != camNum) && (camNum != -100);
			mouse->currentCamController->SwitchTo(showMode);
			mouse->CameraTransition(camTime);
		}
	}
	lua_pop(L, 1);
	
	vector<float> camState;
	int index = 1;
	while (true) {
		lua_pushnumber(L, index);
		lua_gettable(L, -2);
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
	
	return 1;
}


/******************************************************************************/

static int GiveOrder(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 3) || !lua_isnumber(L, -3) ||
	    !lua_istable(L, -2) || !lua_istable(L, -1)) {
		lua_pushstring(L,
			"Incorrect arguments to GiveOrder(id, params[], options[])");
		lua_error(L);
	}

	Command cmd;

	// options
	const int optionTable = lua_gettop(L);
	lua_pushnil(L);
	while (lua_next(L, optionTable) != 0) {
		if (!lua_isstring(L, -1)) {
			lua_pushstring(L,
				"Incorrect option table in GiveOrder(id, params[], options[])");
			lua_error(L);
		}
		const string key = lua_tostring(L, -1);
		if (key == "right") {
			cmd.options |= RIGHT_MOUSE_KEY;
		} else if (key == "alt") {
			cmd.options |= ALT_KEY;
		} else if (key == "ctrl") {
			cmd.options |= CONTROL_KEY;
		} else if (key == "shift") {
			cmd.options |= SHIFT_KEY;
		}	
		lua_pop(L, 1); // pop the value, leave the key for the next iteration
	}
	lua_pop(L, 1); // pop the table

	// params
	const int paramTable = lua_gettop(L);
	lua_pushnil(L);
	while (lua_next(L, paramTable) != 0) {
		if (!lua_isnumber(L, -1) || !lua_isnumber(L, -2)) {
			lua_pushstring(L,
				"Incorrect param table in GiveOrder(id, params[], options[])");
			lua_error(L);
		}
		const float value = (float)lua_tonumber(L, -1);
		cmd.params.push_back(value);
		lua_pop(L, 1); // pop the value, leave the key for the next iteration
	}
	lua_pop(L, 1); // pop the table

	// id	
	cmd.id = (int)lua_tonumber(L, 1);
	lua_pop(L, 1); // pop the id

	selectedUnits.GiveCommand(cmd);
	
	lua_pushboolean(L, true);

	return 1;


	// FIXME -- do some basic checks
	printf("LUA: GiveCommand %i\n", cmd.id);
	for (int i = 0; i < cmd.params.size(); i++) {
		printf("%f\n", cmd.params[i]);
	}
	printf("%s%s%s%s\n",
	       (cmd.options & RIGHT_MOUSE_KEY) ? " right" : "",
	       (cmd.options & ALT_KEY)         ? " alt" : "",
	       (cmd.options & CONTROL_KEY)     ? " ctrl" : "",
	       (cmd.options & SHIFT_KEY)       ? " shift" : "");
	
	int type1 = -1;
	int type2 = -1;

	switch (cmd.id) {
		case CMD_STOP:
		case CMD_WAIT:
		case CMD_SELFD:
		case CMD_STOCKPILE: 
		case CMD_GATHERWAIT: {
			type1 = CMDTYPE_ICON;
			break;
		}
		case CMD_TIMEWAIT:
		case CMD_SQUADWAIT: {
			type1 = CMDTYPE_NUMBER;
			break;
		}
		case CMD_DEATHWAIT: {
			type1 = CMDTYPE_ICON_UNIT_OR_RECTANGLE;
			break;
		}
		case CMD_DGUN:
		case CMD_ATTACK: {
			type1 = CMDTYPE_ICON_UNIT_OR_MAP;
			break;
		}
		case CMD_ONOFF:
		case CMD_CLOAK:
		case CMD_REPEAT:
		case CMD_FIRE_STATE:
		case CMD_MOVE_STATE:
		case CMD_TRAJECTORY:
		case CMD_LOOPBACKATTACK:
		case CMD_AUTOREPAIRLEVEL: {
			type1 = CMDTYPE_ICON_MODE;
			break;
		}
		case CMD_MOVE: {
			type2 = CMDTYPE_ICON_FRONT;
			// fallthru
		}
		case CMD_PATROL:
		case CMD_FIGHT: {
			type1 = CMDTYPE_ICON_MAP;
			break;
		}
		case CMD_RESTORE:
		case CMD_AREA_ATTACK:
		case CMD_UNLOAD_UNITS: {
			type1 = CMDTYPE_ICON_AREA;
			break;
		}
		case CMD_GUARD: {
			type1 = CMDTYPE_ICON_UNIT;
			break;
		}
		case CMD_AISELECT: {
			type1 = CMDTYPE_COMBO_BOX;
			break;
		}
		case CMD_REPAIR:
		case CMD_CAPTURE:
		case CMD_LOAD_UNITS: {
			type1 = CMDTYPE_ICON_UNIT_OR_AREA;
			break;
		}
		case CMD_RECLAIM:
		case CMD_RESURRECT: {
			type1 = CMDTYPE_ICON_UNIT_FEATURE_OR_AREA;
			break;
		}
//		case CMD_GROUPSELECT:
//		case CMD_GROUPADD:
//		case CMD_GROUPCLEAR:
//		case CMD_SETBASE:
//		case CMD_INTERNAL:
//		case CMD_SET_WANTED_MAX_SPEED:
//		case CMD_UNLOAD_UNIT:
	}
	
	lua_pushboolean(L, true);

	return 1;
}


static int GetMapHeight(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isnumber(L, 1) || !lua_isnumber(L, -2)) {
		lua_pushstring(L, "Incorrect arguments to GetMapHeight(x, y");
		lua_error(L);
	}
	const float x = lua_tonumber(L, 1);
	const float y = lua_tonumber(L, 2);
	lua_pushnumber(L, ground->GetHeight(x, y));
	return 1;
}


static int TestBuildOrder(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 5) || !lua_isnumber(L, 1) ||
	    !lua_isnumber(L, 2) || !lua_isnumber(L, 3) ||
	    !lua_isnumber(L, 4) || !lua_isnumber(L, 5)) {
		lua_pushstring(L,
			"Incorrect arguments to TestBuildOrder(unitDefID, x, y, z, facing");
		lua_error(L);
	}
	const int unitDefID = (int)lua_tonumber(L, 1);
	if ((unitDefID < 0) || (unitDefID > unitDefHandler->numUnits)) {
		lua_pushboolean(L, 0);
		return 1;
	}
	
	const float3 pos(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
	const int facing = (int)lua_tonumber(L, 5);

	BuildInfo bi;
	bi.buildFacing = facing;
	bi.def = unitDefHandler->GetUnitByID(unitDefID);
	bi.pos = helper->Pos2BuildPos(bi);
	CFeature* feature;
	if(!uh->TestUnitBuildSquare(bi, feature, gu->myAllyTeam)) {
		lua_pushboolean(L, 0);
	} else {
		lua_pushboolean(L, 1);
	}
	return 1;		
}


/******************************************************************************/

static int SetShareLevel(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isstring(L, 1) || !lua_isnumber(L, 2)) {
		lua_pushstring(L, "Incorrect arguments to SetShareLevels(metal, energy");
		lua_error(L);
	}
	
	const string shareType = lua_tostring(L, 1);
	const float shareLevel = max(0.0f, min(1.0f, (float)lua_tonumber(L, 2)));
	
	if (shareType == "metal") {
		net->SendData<unsigned char, float, float>(NETMSG_SETSHARE,
			gu->myTeam, shareLevel, gs->Team(gu->myTeam)->energyShare);
	}
	else if (shareType == "energy") {
		net->SendData<unsigned char, float, float>(NETMSG_SETSHARE,
			gu->myTeam, gs->Team(gu->myTeam)->metalShare, shareLevel);
	}
	else {
		logOutput.Print("SetShareLevel() unknown resource: %s", shareType.c_str());
	}
	return 0;
}


/******************************************************************************/

static int AddMapIcon(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 4) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) ||
	    !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		lua_pushstring(L, "Incorrect arguments to AddMapIcon(id, x, y, z");
		lua_error(L);
	}
	const int cmdID = (int)lua_tonumber(L, 1);
	const float3 pos(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
	cursorIcons.AddIcon(cmdID, pos);
	return 0;
}


static int AddMapText(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 4) ||
	    !lua_isstring(L, 1) || !lua_isnumber(L, 2) ||
	    !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		lua_pushstring(L, "Incorrect arguments to AddMapIcon(text, x, y, z");
		lua_error(L);
	}
	const string text = lua_tostring(L, 1);
	const float3 pos(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
	cursorIcons.AddIconText(text, pos);
	return 0;
}


static int AddMapUnit(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 6) ||
	    !lua_isstring(L, 1) || !lua_isnumber(L, 2) ||
	    !lua_isnumber(L, 3) || !lua_isnumber(L, 4) ||
	    !lua_isnumber(L, 5) || !lua_isnumber(L, 6)) {
		lua_pushstring(L,
			"Incorrect arguments to AddMapUnit(unitDefID, x, y, z, team, facing");
		lua_error(L);
	}
	const int unitDefID = (int)lua_tonumber(L, 1);
	if ((unitDefID < 0) || (unitDefID > unitDefHandler->numUnits)) {
		return 0;
	}
	const float3 pos(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
	const int team = (int)lua_tonumber(L, 5);
	if ((team < 0) || (team >= gs->activeTeams)) {
		return 0;
	}
	const int facing = (int)lua_tonumber(L, 6);
	cursorIcons.AddBuildIcon(-unitDefID, pos, team, facing);
	return 0;
}


/******************************************************************************/

static int DrawText(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 3) ||
	    !lua_isstring(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
		lua_pushstring(L,
			"Incorrect arguments to "
			"DrawText(msg, x, y [,size] [,face] [,color]");
		lua_error(L);
	}
	const string text = lua_tostring(L, 1);
	const float x     = lua_tonumber(L, 2);
	const float y     = lua_tonumber(L, 3);
	
	float size = 12.0f;
	bool right = false;
	bool center = false;
	bool outline = false;
	
	if ((args >= 4) && lua_isnumber(L, 4)) {
		size = lua_tonumber(L, 4);
	}
	if ((args >= 5) && lua_isstring(L, 5)) {
		const string opts = lua_tostring(L, 5);
		if (strstr(opts.c_str(), "r") != NULL) { right = true; }
		if (strstr(opts.c_str(), "c") != NULL) { center = true; }
		if (strstr(opts.c_str(), "o") != NULL) { outline = true; }
	}

	const float tHeight = font->CalcTextHeight(text.c_str());
	const float yScale = size / tHeight;
	const float xScale = yScale;
	float xj = x; // justified x position
	if (right) {
		xj -= xScale * font->CalcTextWidth(text.c_str());
	} else if (center) {
		xj -= xScale * font->CalcTextWidth(text.c_str()) * 0.5f;
	}
	glPushMatrix();
	glTranslatef(xj, y, 0.0f);
	glScalef(xScale, yScale, 1.0f);
	if (!outline) {
		font->glPrintColor("%s", text.c_str());
	} else {
		const float dark[4] = { 0.25f, 0.25f, 0.25f, 0.8f };
		const float noshow[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		const float xPixel = 1.0f / xScale;
		const float yPixel = 1.0f / yScale;
		font->glPrintOutlined(text.c_str(), xPixel, yPixel, noshow, dark);
		font->glPrintColor("%s", text.c_str());
	}
	glPopMatrix();

	return 0;	
}


/******************************************************************************/

struct VertexData {
	float vert[3];
	float norm[3];
	float txcd[2];
	float color[4];
	bool hasVert;
	bool hasNorm;
	bool hasTxcd;
	bool hasColor;
};


static int ParseFloatArray(lua_State* L, float* array, int size)
{
	if (!lua_istable(L, -1)) {
		return -1;
	}
	int index = 0;
	const int table = lua_gettop(L);
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_isnumber(L, -1)) {
			logOutput.Print("DrawShape: bad array value\n");
			lua_pop(L, 2); // pop the value and the key
			return -1;
		}
		if (index < size) {
			array[index] = (float)lua_tonumber(L, -1);
			index++;
		}
	}
	return index;
}


static bool ParseVertexData(lua_State* L, VertexData& vd)
{
	if (!lua_istable(L, -1)) {
		return false;
	}
	
	vd.hasVert = vd.hasNorm = vd.hasTxcd = vd.hasColor = false;

	const int table = lua_gettop(L);
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_istable(L, -1) || !lua_isstring(L, -2)) {
			logOutput.Print("DrawShape: bad vertex data row\n");
			lua_pop(L, 2); // pop the value and key
			return false;
		}

		const string key = lua_tostring(L, -2);

		if ((key == "v") || (key == "vertex")) {
			vd.vert[0] = 0.0f; vd.vert[1] = 0.0f; vd.vert[2] = 0.0f;
			if (ParseFloatArray(L, vd.vert, 3) >= 2) {
				vd.hasVert = true;
			} else {
				logOutput.Print("DrawShape: bad vertex array\n");
				lua_pop(L, 2); // pop the value and key
				return false;
			}
		}
		else if ((key == "n") || (key == "normal")) {
			vd.norm[0] = 0.0f; vd.norm[1] = 1.0f; vd.norm[2] = 0.0f;
			if (ParseFloatArray(L, vd.norm, 3) == 3) {
				vd.hasNorm = true;
			} else {
				logOutput.Print("DrawShape: bad normal array\n");
				lua_pop(L, 2); // pop the value and key
				return false;
			}
		}
		else if ((key == "t") || (key == "texcoord")) {
			vd.txcd[0] = 0.0f; vd.txcd[1] = 0.0f;
			if (ParseFloatArray(L, vd.txcd, 2) == 2) {
				vd.hasTxcd = true;
			} else {
				logOutput.Print("DrawShape: bad texcoord array\n");
				lua_pop(L, 2); // pop the value and key
				return false;
			}
		}
		else if ((key == "c") || (key == "color")) {
			vd.color[0] = 1.0f; vd.color[1] = 1.0f;
			vd.color[2] = 1.0f; vd.color[3] = 1.0f;
			if (ParseFloatArray(L, vd.color, 4) >= 0) {
				vd.hasColor = true;
			} else {
				logOutput.Print("DrawShape: bad color array\n");
				lua_pop(L, 2); // pop the value and key
				return false;
			}
		}
	}

	return vd.hasVert;
}


static int DrawShape(lua_State* L)
{	
	if (!drawingEnabled) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args != 3) ||
	    !lua_isnumber(L, 1) || !lua_isstring(L, 2) || !lua_istable(L, 3)) {
		lua_pushstring(L,
			"Incorrect arguments to DrawShape(type, texture, verts[])");
		lua_error(L);
	}

	const string texture = lua_tostring(L, 2);
	if (texture.empty()) {
		glDisable(GL_TEXTURE_2D);
	} else {
		if (!guihandler->BindNamedTexture(texture)) {
			return 0;
		}
		glEnable(GL_TEXTURE_2D);
	}

	const GLuint type = (GLuint)lua_tonumber(L, 1);
	switch (type) {
		case GL_POINTS:
		case GL_LINES:
		case GL_LINE_STRIP:
		case GL_LINE_LOOP:
		case GL_TRIANGLES:
		case GL_TRIANGLE_STRIP:
		case GL_TRIANGLE_FAN:
		case GL_QUADS:
		case GL_QUAD_STRIP:
		case GL_POLYGON:{
			glBegin(type);
			break;
		}
		default: { return 0; }
	}
	const int table = lua_gettop(L);
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		VertexData vd;
		if (!ParseVertexData(L, vd)) {
			logOutput.Print("DrawShape: bad vertex data\n");
			break;
		}
		if (vd.hasColor) { glColor4fv(vd.color);   }
		if (vd.hasTxcd)  { glTexCoord2fv(vd.txcd); }
		if (vd.hasNorm)  { glNormal3fv(vd.norm);   }
		if (vd.hasVert)  { glVertex3fv(vd.vert);   } // always last
	}

	glEnd();

	return 0;
}


/******************************************************************************/

static int DrawState(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_istable(L, 1)) {
		lua_pushstring(L,
			"Incorrect arguments to DrawState(table)");
		lua_error(L);
	}
	
	const int table = lua_gettop(L);
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_isstring(L, -2)) { // the key
			logOutput.Print("DrawState: bad state type\n");
			break;
		}
		const string key = lua_tostring(L, -2);

		if (key == "reset") {
			if (screenTransform) {
				glDisable(GL_DEPTH_TEST);
			} else {
				glEnable(GL_DEPTH_TEST);
			}
			glDepthMask(GL_FALSE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDisable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER, 0.5f);
			glDisable(GL_LIGHTING);
			glDisable(GL_COLOR_LOGIC_OP);
			glLogicOp(GL_INVERT);
			glDisable(GL_CULL_FACE);
			glCullFace(GL_BACK);
		}
		else if (key == "depthmask") {
			if (!lua_isboolean(L, -1)) {
			} else {
				glDepthMask(lua_toboolean(L, -1) ? GL_TRUE : GL_FALSE);
			}
		}
		else if (key == "depthtest") {
			if (!lua_isboolean(L, -1)) {
			} else {
				if (lua_toboolean(L, -1)) {
					glEnable(GL_DEPTH_TEST);
				} else {
					glDisable(GL_DEPTH_TEST);
				}
			}
		}
		else if (key == "lighting") {
			if (!lua_isboolean(L, -1)) {
			} else {
				if (lua_toboolean(L, -1)) {
					glEnable(GL_LIGHTING);
				} else {
					glDisable(GL_LIGHTING);
				}
			}
		}
		else if (key == "blending") {
			float values[2];
			const int count = ParseFloatArray(L, values, 2);
			if (count == 0) {
				glDisable(GL_BLEND);
			}
			else if (count == 2) {
				glEnable(GL_BLEND);
				glBlendFunc((GLenum)values[0], (GLenum)values[1]);
			}
			else {
			}
		}
		else if (key == "culling") {
			float face;
			const int count = ParseFloatArray(L, &face, 1);
			if (count == 0) {
				glDisable(GL_CULL_FACE);
			}
			else if (count == 1) {
				glEnable(GL_CULL_FACE);
				glCullFace((GLenum)face);
			}
			else {
			}
		}
		else if (key == "alphatest") {
			float values[2];
			const int count = ParseFloatArray(L, values, 2);
			if (count == 0) {
				glDisable(GL_ALPHA_TEST);
			}
			else if (count == 2) {
				glEnable(GL_ALPHA_TEST);
				glAlphaFunc((GLenum)values[0], values[1]);
			}
			else {
			}
		}
		else if (key == "logicop") {
			float op;
			const int count = ParseFloatArray(L, &op, 1);
			if (count == 0) {
				glDisable(GL_COLOR_LOGIC_OP);
			}
			else if (count == 1) {
				glEnable(GL_COLOR_LOGIC_OP);
				glLogicOp((GLenum)op);
			}
			else {
			}
		}
		else {
			logOutput.Print("DrawState: unknown state type: %s\n", key.c_str());
		}
	}
	
	return 0;
}


/******************************************************************************/

static int DrawTranslate(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args != 3) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
		lua_pushstring(L, "Incorrect arguments to DrawTranslate(x, y, z)");
		lua_error(L);
	}
	const float x = (float)lua_tonumber(L, 1);
	const float y = (float)lua_tonumber(L, 2);
	const float z = (float)lua_tonumber(L, 3);
	glTranslatef(x, y, z);
	return 0;
}


static int DrawScale(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args != 3) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
		lua_pushstring(L, "Incorrect arguments to DrawScale(x, y, z)");
		lua_error(L);
	}
	const float x = (float)lua_tonumber(L, 1);
	const float y = (float)lua_tonumber(L, 2);
	const float z = (float)lua_tonumber(L, 3);
	glScalef(x, y, z);
	return 0;
}


static int DrawRotate(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args != 4) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) ||
	    !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		lua_pushstring(L, "Incorrect arguments to DrawRotate(r, x, y, z)");
		lua_error(L);
	}
	const float r = (float)lua_tonumber(L, 1);
	const float x = (float)lua_tonumber(L, 2);
	const float y = (float)lua_tonumber(L, 3);
	const float z = (float)lua_tonumber(L, 4);
	glRotatef(r, x, y, z);
	return 0;
}


/******************************************************************************/

static int DrawPushMatrix(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "DrawPushMatrix takes no arguments");
		lua_error(L);
	}

	if (matrixDepth >= maxMatrixDepth) {
		lua_pushstring(L, "DrawPushMatrix: matrix overflow");
		lua_error(L);
	}
		
	glPushMatrix();
	matrixDepth++;			

	return 0;
}


static int DrawPopMatrix(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "DrawPopMatrix takes no arguments");
		lua_error(L);
	}

	if (matrixDepth <= 0) {
		lua_pushstring(L, "DrawPopMatrix: matrix underflow");
		lua_error(L);
	}
		
	glPopMatrix();
	matrixDepth--;			

	return 0;
}

/******************************************************************************/

static int DrawListCreate(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isfunction(L, 1)) {
		lua_pushstring(L,
			"Incorrect arguments to DrawListCreate(func [, arg1, arg2, etc ...])");
		lua_error(L);
	}
	
	// save the current state
	const bool origDrawingEnabled = drawingEnabled;
	drawingEnabled = true;
	const int	origMatrixDepth = matrixDepth;
	matrixDepth = 0;
	
	const GLuint list = glGenLists(1);
	glNewList(list, GL_COMPILE);
	const int error = lua_pcall(L, args - 1, 0, 0);
	glEndList();

	if (error != 0) {
		glDeleteLists(list, 1);
		logOutput.Print("DrawListCreate: error = %s\n", lua_tostring(L, -1));
		lua_pushnumber(L, -1);
	} else {
		displayLists.push_back(list);
		lua_pushnumber(L, displayLists.size() - 1);
	}

	// restore the state	
	matrixDepth = origMatrixDepth;
	drawingEnabled = origDrawingEnabled;
	
	return 1;
}


static int DrawListRun(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1)) {
		lua_pushstring(L,
			"Incorrect arguments to DrawListRun(list)");
		lua_error(L);
	}
	const GLuint listIndex = (GLuint)lua_tonumber(L, 1);
	if (listIndex < displayLists.size()) {
		glCallList(displayLists[listIndex]);
	}
	return 0;
}


static int DrawListDelete(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1)) {
		lua_pushstring(L,
			"Incorrect arguments to DrawListDelete(list)");
		lua_error(L);
	}
	const GLuint listIndex = (GLuint)lua_tonumber(L, 1);
	if (listIndex < displayLists.size()) {
		glDeleteLists(displayLists[listIndex], 1);
		displayLists[listIndex] = 0;
	}
	return 0;
}


/******************************************************************************/

static int PlaySoundFile(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1)) {
		lua_pushstring(L,
			"Incorrect arguments to PlaySoundFile(soundname [,volume])");
		lua_error(L);
	}
	const string soundFile = lua_tostring(L, 1);
	const unsigned int soundID = sound->GetWaveId(soundFile);
	if (soundID > 0) {
		float volume = 1.0f;
		if (args >= 2) {
			volume = (float)lua_tonumber(L, 2);
		}
		sound->PlaySample(soundID, volume);
		lua_pushboolean(L, 1);
	} else {
		lua_pushboolean(L, 0);
	}
	return 1;
}


/******************************************************************************/
