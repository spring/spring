#include "StdAfx.h"
// LuaUI.cpp: implementation of the CLuaUI class.
//
//////////////////////////////////////////////////////////////////////

#include "LuaUI.h"
#include <set>
#include <cctype>
#include <SDL_keysym.h>
#include <SDL_mouse.h>
extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}
#include "LuaState.h"
#include "ExternalAI/GlobalAIHandler.h"
#include "ExternalAI/Group.h"
#include "ExternalAI/GroupHandler.h"
#include "Game/command.h"
#include "Game/Camera.h"
#include "Game/CameraController.h"
#include "Game/Game.h"
#include "Game/GameHelper.h"
#include "Game/SelectedUnits.h"
#include "Game/PlayerRoster.h"
#include "Game/Team.h"
#include "Game/UI/CommandColors.h"
#include "Game/UI/CursorIcons.h"
#include "Game/UI/GuiHandler.h"
#include "Game/UI/KeyCodes.h"
#include "Game/UI/KeySet.h"
#include "Game/UI/KeyBindings.h"
#include "Game/UI/MouseHandler.h"
#include "Map/Ground.h"
#include "Rendering/glFont.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/UnitModels/3DModelParser.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Sim/Misc/Feature.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/FactoryCAI.h"
#include "Sim/Units/CommandAI/LineDrawer.h"
#include "Sim/Weapons/Weapon.h"
#include "System/LogOutput.h"
#include "System/Net.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/Platform/ConfigHandler.h"
#include "System/Platform/FileSystem.h"
#include "System/Sound.h"

extern Uint8 *keys;
extern GLfloat LightDiffuseLand[];
extern GLfloat LightAmbientLand[];


/******************************************************************************/

// LUA C functions  (placed in the "Spring" table)
   
static int LoadTextVFS(lua_State* L);
static int GetDirListVFS(lua_State* L);
static int GetDirList(lua_State* L);

static int GetConfigInt(lua_State* L);
static int SetConfigInt(lua_State* L);
static int GetConfigString(lua_State* L);
static int SetConfigString(lua_State* L);

static int SetUnitDefIcon(lua_State* L);

static int GetFPS(lua_State* L);
static int GetGameSeconds(lua_State* L);
static int GetLastFrameSeconds(lua_State* L);

static int GetActiveCommand(lua_State* L);
static int GetDefaultCommand(lua_State* L);

static int GetMouseState(lua_State* L);
static int GetMouseCursor(lua_State* L);
static int SetMouseCursor(lua_State* L);

static int GetKeyState(lua_State* L);
static int GetModKeyState(lua_State* L);
static int GetPressedKeys(lua_State* L);

static int GetCurrentTooltip(lua_State* L);

static int GetKeyCode(lua_State* L);
static int GetKeySymbol(lua_State* L);
static int GetKeyBindings(lua_State* L);
static int GetActionHotKeys(lua_State* L);

static int SendCommands(lua_State* L);
static int GiveOrder(lua_State* L);

static int GetGroupList(lua_State* L);
static int GetSelectedGroup(lua_State* L);
static int GetGroupAIName(lua_State* L);
static int GetGroupAIList(lua_State* L);

static int GetSelectedUnits(lua_State* L);
static int GetSelectedUnitsSorted(lua_State* L);
static int GetSelectedUnitsCounts(lua_State* L);
static int GetGroupUnits(lua_State* L);
static int GetGroupUnitsSorted(lua_State* L);
static int GetGroupUnitsCounts(lua_State* L);
static int GetTeamUnits(lua_State* L);
static int GetTeamUnitsSorted(lua_State* L);
static int GetTeamUnitsCounts(lua_State* L);

static int GetUnitsInBox(lua_State* L);
static int GetUnitsInPlanes(lua_State* L);
static int GetUnitsInCylinder(lua_State* L);

static int GetUnitDefID(lua_State* L);
static int GetUnitTeam(lua_State* L);
static int GetUnitAllyTeam(lua_State* L);
static int GetUnitHealth(lua_State* L);
static int GetUnitResources(lua_State* L);
static int GetUnitExperience(lua_State* L);
static int GetUnitStates(lua_State* L);
static int GetUnitStockpile(lua_State* L);
static int GetUnitRadius(lua_State* L);
static int GetUnitPosition(lua_State* L);
static int GetUnitHeading(lua_State* L);
static int GetUnitBuildFacing(lua_State* L);

static int GetUnitDefDimensions(lua_State* L);

static int GetCommandQueue(lua_State* L);
static int GetFullBuildQueue(lua_State* L);
static int GetRealBuildQueue(lua_State* L);

static int GetMyAllyTeamID(lua_State* L);
static int GetMyTeamID(lua_State* L);
static int GetMyPlayerID(lua_State* L);

static int GetAllyTeamList(lua_State* L);
static int GetTeamList(lua_State* L);
static int GetPlayerList(lua_State* L);
static int GetPlayerRoster(lua_State* L);

static int GetTeamInfo(lua_State* L);
static int GetTeamResources(lua_State* L);
static int GetTeamUnitStats(lua_State* L);
static int SetShareLevel(lua_State* L);
static int ShareResources(lua_State* L);

static int GetPlayerInfo(lua_State* L);

static int AreTeamsAllied(lua_State* L);
static int ArePlayersAllied(lua_State* L);

static int GetCameraState(lua_State* L);
static int SetCameraState(lua_State* L);
static int GetCameraPos(lua_State* L);
static int GetCameraVectors(lua_State* L);
static int WorldToScreenCoords(lua_State* L);
static int TraceScreenRay(lua_State* L);

static int GetGroundHeight(lua_State* L);
static int TestBuildOrder(lua_State* L);
static int Pos2BuildPos(lua_State* L);

static int PlaySoundFile(lua_State* L);

static int AddWorldIcon(lua_State* L);
static int AddWorldText(lua_State* L);
static int AddWorldUnit(lua_State* L);

static int DrawScreenGeometry(lua_State* L);
static int DrawResetState(lua_State* L);
static int DrawClear(lua_State* L);

static int DrawLighting(lua_State* L);
static int DrawShadeModel(lua_State* L);
static int DrawScissor(lua_State* L);
static int DrawDepthMask(lua_State* L);
static int DrawDepthTest(lua_State* L);
static int DrawCulling(lua_State* L);
static int DrawLogicOp(lua_State* L);
static int DrawBlending(lua_State* L);
static int DrawAlphaTest(lua_State* L);
static int DrawLineStipple(lua_State* L);
static int DrawPolygonMode(lua_State* L);
static int DrawPolygonOffset(lua_State* L);

static int DrawTexture(lua_State* L);
static int DrawMaterial(lua_State* L);
static int DrawColor(lua_State* L);

static int DrawLineWidth(lua_State* L);
static int DrawPointSize(lua_State* L);

static int DrawFreeTexture(lua_State* L);

static int DrawShape(lua_State* L);
static int DrawUnitDef(lua_State* L);
static int DrawText(lua_State* L);
static int DrawGetTextWidth(lua_State* L);

static int DrawMatrixMode(lua_State* L);
static int DrawLoadIdentity(lua_State* L);
static int DrawTranslate(lua_State* L);
static int DrawScale(lua_State* L);
static int DrawRotate(lua_State* L);
static int DrawPushMatrix(lua_State* L);
static int DrawPopMatrix(lua_State* L);

static int DrawListCreate(lua_State* L);
static int DrawListRun(lua_State* L);
static int DrawListDelete(lua_State* L);


/******************************************************************************/

// Miscellaneous Local Routines

static void ResetGLState();
static int ParseFloatArray(lua_State* L, float* array, int size);


/******************************************************************************/

// Local Variables

static bool drawingEnabled = false;

static float screenWidth = 0.36f;
static float screenDistance = 0.60f;

static int matrixDepth = 0;
static const int maxMatrixDepth = 16;

static vector<unsigned int> displayLists;

static set<UnitDef*> commanderDefs;

static float textHeight = 0.001f;


/******************************************************************************/

CLuaUI* CLuaUI::GetHandler(const string& filename)
{
	CLuaUI* layout = new CLuaUI();

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

CLuaUI::CLuaUI()
{
	for (int i = 0; i <= unitDefHandler->numUnits; i++) {
		UnitDef* ud = unitDefHandler->GetUnitByID(i);
		if (ud == NULL) {
			continue;
		}
		if (ud->isCommander ||
		    (StringToLower(ud->TEDClassString) == "commander")) {
			commanderDefs.insert(ud);
		}
	}

	// calculate the text height that we'll use to
	// provide a consistent display when rendering text
 	// (note the missing characters)
	textHeight = font->CalcTextHeight("abcdef hi klmno  rstuvwx z"
	                                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
																    "0123456789");
	textHeight = max(1.0e-6f, textHeight);  // safety for dividing
}


CLuaUI::~CLuaUI()
{
	Shutdown();
	for (int i = 0; i < (int)displayLists.size(); i++) {
		glDeleteLists(displayLists[i], 1);
	}
	displayLists.clear();
}


string CLuaUI::LoadFile(const string& filename)
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


bool CLuaUI::LoadCFunctions(lua_State* L)
{
	lua_newtable(L);

#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(LoadTextVFS);
	REGISTER_LUA_CFUNC(GetDirListVFS);
	REGISTER_LUA_CFUNC(GetDirList);
	REGISTER_LUA_CFUNC(SendCommands);
	REGISTER_LUA_CFUNC(GiveOrder);
	REGISTER_LUA_CFUNC(GetFPS);
	REGISTER_LUA_CFUNC(GetGameSeconds);
	REGISTER_LUA_CFUNC(GetLastFrameSeconds);
	REGISTER_LUA_CFUNC(GetActiveCommand);
	REGISTER_LUA_CFUNC(GetDefaultCommand);
	REGISTER_LUA_CFUNC(GetMouseState);
	REGISTER_LUA_CFUNC(GetMouseCursor);
	REGISTER_LUA_CFUNC(SetMouseCursor);
	REGISTER_LUA_CFUNC(GetKeyState);
	REGISTER_LUA_CFUNC(GetModKeyState);
	REGISTER_LUA_CFUNC(GetPressedKeys);
	REGISTER_LUA_CFUNC(GetCurrentTooltip);
	REGISTER_LUA_CFUNC(GetKeyCode);
	REGISTER_LUA_CFUNC(GetKeySymbol);
	REGISTER_LUA_CFUNC(GetKeyBindings);
	REGISTER_LUA_CFUNC(GetActionHotKeys);
	REGISTER_LUA_CFUNC(GetConfigInt);
	REGISTER_LUA_CFUNC(SetConfigInt);
	REGISTER_LUA_CFUNC(GetConfigString);
	REGISTER_LUA_CFUNC(SetConfigString);
	REGISTER_LUA_CFUNC(SetUnitDefIcon);
	REGISTER_LUA_CFUNC(GetGroupList);
	REGISTER_LUA_CFUNC(GetSelectedGroup);
	REGISTER_LUA_CFUNC(GetGroupAIName);
	REGISTER_LUA_CFUNC(GetGroupAIList);
	REGISTER_LUA_CFUNC(GetSelectedUnits);
	REGISTER_LUA_CFUNC(GetSelectedUnitsSorted);
	REGISTER_LUA_CFUNC(GetSelectedUnitsCounts);
	REGISTER_LUA_CFUNC(GetGroupUnits);
	REGISTER_LUA_CFUNC(GetGroupUnitsSorted);
	REGISTER_LUA_CFUNC(GetGroupUnitsCounts);
	REGISTER_LUA_CFUNC(GetTeamUnits);
	REGISTER_LUA_CFUNC(GetTeamUnitsSorted);
	REGISTER_LUA_CFUNC(GetTeamUnitsCounts);
	REGISTER_LUA_CFUNC(GetUnitsInBox);
	REGISTER_LUA_CFUNC(GetUnitsInPlanes);
	REGISTER_LUA_CFUNC(GetUnitsInCylinder);
	REGISTER_LUA_CFUNC(GetUnitDefID);
	REGISTER_LUA_CFUNC(GetUnitTeam);
	REGISTER_LUA_CFUNC(GetUnitAllyTeam);
	REGISTER_LUA_CFUNC(GetUnitHealth);
	REGISTER_LUA_CFUNC(GetUnitResources);
	REGISTER_LUA_CFUNC(GetUnitExperience);
	REGISTER_LUA_CFUNC(GetUnitStates);
	REGISTER_LUA_CFUNC(GetUnitStockpile);
	REGISTER_LUA_CFUNC(GetUnitRadius);
	REGISTER_LUA_CFUNC(GetUnitPosition);
	REGISTER_LUA_CFUNC(GetUnitHeading);
	REGISTER_LUA_CFUNC(GetUnitBuildFacing);
	REGISTER_LUA_CFUNC(GetUnitDefDimensions);
	REGISTER_LUA_CFUNC(GetCommandQueue);
	REGISTER_LUA_CFUNC(GetFullBuildQueue);
	REGISTER_LUA_CFUNC(GetRealBuildQueue);
	REGISTER_LUA_CFUNC(GetMyAllyTeamID);
	REGISTER_LUA_CFUNC(GetMyTeamID);
	REGISTER_LUA_CFUNC(GetMyPlayerID);
	REGISTER_LUA_CFUNC(GetAllyTeamList);
	REGISTER_LUA_CFUNC(GetTeamList);
	REGISTER_LUA_CFUNC(GetPlayerList);
	REGISTER_LUA_CFUNC(GetPlayerRoster);
	REGISTER_LUA_CFUNC(GetTeamInfo);
	REGISTER_LUA_CFUNC(GetTeamResources);
	REGISTER_LUA_CFUNC(GetTeamUnitStats);
	REGISTER_LUA_CFUNC(SetShareLevel);
	REGISTER_LUA_CFUNC(ShareResources);
	REGISTER_LUA_CFUNC(GetPlayerInfo);
	REGISTER_LUA_CFUNC(AreTeamsAllied);
	REGISTER_LUA_CFUNC(ArePlayersAllied);
	REGISTER_LUA_CFUNC(GetCameraState);
	REGISTER_LUA_CFUNC(SetCameraState);
	REGISTER_LUA_CFUNC(GetCameraPos);
	REGISTER_LUA_CFUNC(GetCameraVectors);
	REGISTER_LUA_CFUNC(WorldToScreenCoords);
	REGISTER_LUA_CFUNC(TraceScreenRay);
	REGISTER_LUA_CFUNC(GetGroundHeight);
	REGISTER_LUA_CFUNC(TestBuildOrder);
	REGISTER_LUA_CFUNC(Pos2BuildPos);
	REGISTER_LUA_CFUNC(AddWorldIcon);
	REGISTER_LUA_CFUNC(AddWorldText);
	REGISTER_LUA_CFUNC(AddWorldUnit);
	REGISTER_LUA_CFUNC(PlaySoundFile);

#define REGISTER_LUA_DRAW_CFUNC(x) \
	lua_pushstring(L, #x);           \
	lua_pushcfunction(L, Draw ## x); \
	lua_rawset(L, -3)

	lua_pushstring(L, "Draw");
	lua_newtable(L);
	REGISTER_LUA_DRAW_CFUNC(ScreenGeometry);
	REGISTER_LUA_DRAW_CFUNC(ResetState);
	REGISTER_LUA_DRAW_CFUNC(Clear);
	REGISTER_LUA_DRAW_CFUNC(Lighting);
	REGISTER_LUA_DRAW_CFUNC(ShadeModel);
	REGISTER_LUA_DRAW_CFUNC(Scissor);
	REGISTER_LUA_DRAW_CFUNC(DepthMask);
	REGISTER_LUA_DRAW_CFUNC(DepthTest);
	REGISTER_LUA_DRAW_CFUNC(Culling);
	REGISTER_LUA_DRAW_CFUNC(LogicOp);
	REGISTER_LUA_DRAW_CFUNC(Blending);
	REGISTER_LUA_DRAW_CFUNC(AlphaTest);
	REGISTER_LUA_DRAW_CFUNC(LineStipple);
	REGISTER_LUA_DRAW_CFUNC(PolygonMode);
	REGISTER_LUA_DRAW_CFUNC(PolygonOffset);

	REGISTER_LUA_DRAW_CFUNC(Texture);
	REGISTER_LUA_DRAW_CFUNC(Material);
	REGISTER_LUA_DRAW_CFUNC(Color);

	REGISTER_LUA_DRAW_CFUNC(LineWidth);
	REGISTER_LUA_DRAW_CFUNC(PointSize);

	REGISTER_LUA_DRAW_CFUNC(FreeTexture);

	REGISTER_LUA_DRAW_CFUNC(Shape);
	REGISTER_LUA_DRAW_CFUNC(UnitDef);
	REGISTER_LUA_DRAW_CFUNC(Text);
	REGISTER_LUA_DRAW_CFUNC(GetTextWidth);

	REGISTER_LUA_DRAW_CFUNC(ListCreate);
	REGISTER_LUA_DRAW_CFUNC(ListRun);
	REGISTER_LUA_DRAW_CFUNC(ListDelete);

	REGISTER_LUA_DRAW_CFUNC(MatrixMode);
	REGISTER_LUA_DRAW_CFUNC(LoadIdentity);
	REGISTER_LUA_DRAW_CFUNC(Translate);
	REGISTER_LUA_DRAW_CFUNC(Scale);
	REGISTER_LUA_DRAW_CFUNC(Rotate);
	REGISTER_LUA_DRAW_CFUNC(PushMatrix);
	REGISTER_LUA_DRAW_CFUNC(PopMatrix);
	lua_rawset(L, -3); // add the Draw table

	lua_setglobal(L, "Spring");

	return true;
}


bool CLuaUI::LoadCode(lua_State* L,
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


/******************************************************************************/

void CLuaUI::Shutdown()
{
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return;
	}
	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "Shutdown");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return;
	}

	const int error = lua_pcall(L, 0, 0, 0);
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n", error,
		                "Call_Shutdown", lua_tostring(L, -1));
		lua_pop(L, 1);
		return;
	}
	return;
}


bool CLuaUI::ConfigCommand(const string& command)
{
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "ConfigureLayout");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return true; // the call is not defined
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


bool CLuaUI::UpdateLayout(bool commandsChanged, int activePage)
{
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "UpdateLayout");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return false; // the call is not defined
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
	const bool forceLayout = !!lua_toboolean(L, -1);
	lua_pop(L, 1);
	return forceLayout;
}


bool CLuaUI::CommandNotify(const Command& cmd)
{
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "CommandNotify");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
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
		return false;
	}

	return !!lua_toboolean(L, -1);
}


bool CLuaUI::UnitCreated(CUnit* unit)
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
		return true; // the call is not defined
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


bool CLuaUI::UnitReady(CUnit* unit, CUnit* builder)
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
		return true; // the call is not defined
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


bool CLuaUI::UnitDestroyed(CUnit* victim, CUnit* attacker)
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
		return true; // the call is not defined
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


bool CLuaUI::UnitChangedTeam(CUnit* unit, int oldTeam, int newTeam)
{
	if ((gu->myAllyTeam != gs->AllyTeam(oldTeam)) &&
	    (gu->myAllyTeam != gs->AllyTeam(newTeam))) {
		return false;
	}

	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "UnitChangedTeam");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return true; // the call is not defined
	}

	lua_pushnumber(L, unit->id);	
	lua_pushnumber(L, unit->unitDef->id);	
	lua_pushnumber(L, oldTeam);	
	lua_pushnumber(L, newTeam);	

	// call the routine
	const int error = lua_pcall(L, 4, 0, 0);
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n", error,
		                "Call_UnitChangedTeam", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}

	return true;
}


bool CLuaUI::GroupChanged(int groupID)
{
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "GroupChanged");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return true; // the call is not defined
	}

	lua_pushnumber(L, groupID);	

	// call the routine
	const int error = lua_pcall(L, 1, 0, 0);
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n", error,
		                "Call_GroupChanged", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}

	return true;
}


bool CLuaUI::DrawWorldItems()
{
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "DrawWorldItems");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return true; // the call is not defined
	}

	drawingEnabled = true;

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	// push the current GL state
	glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT |
	             GL_POLYGON_BIT | GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT |
	             GL_LINE_BIT | GL_POINT_BIT);

	// setup a known state
	ResetGLState();
	glEnable(GL_NORMALIZE);

	glPushMatrix();

	matrixDepth = 0;

	// call the routine
	bool retval = true;
	const int error = lua_pcall(L, 0, 0, 0);
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n", error,
		                "Call_DrawWorldItems", lua_tostring(L, -1));
		lua_pop(L, 1);
		retval = false;
	}

	if (matrixDepth != 0) {
		logOutput.Print("DrawWorldItems: matrix mismatch");
		while (matrixDepth > 0) {
			glPopMatrix();
			matrixDepth--;
		}
	}

	glPopMatrix();

	glDisable(GL_NORMALIZE);
	
	// pop the GL state
	glPopAttrib();

	drawingEnabled = false;

	return retval;
}


static void SetupScreenTransform()
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	const float dist   = screenDistance;         // eye-to-screen (meters)
	const float width  = screenWidth;            // screen width (meters)
	const float vppx   = float(gu->winPosX + gu->viewPosX); // view pixel pos x
	const float vppy   = float(gu->winPosY + gu->viewPosY); // view pixel pos y
	const float vpsx   = float(gu->viewSizeX);   // view pixel size x
	const float vpsy   = float(gu->viewSizeY);   // view pixel size y
	const float spsx   = float(gu->screenSizeX); // screen pixel size x
	const float spsy   = float(gu->screenSizeY); // screen pixel size x
	const float halfSX = 0.5f * spsx;            // half screen pixel size x
	const float halfSY = 0.5f * spsy;            // half screen pixel size y

	const float zplane = dist * (spsx / width);
	const float znear  = zplane * 0.5;
	const float zfar   = zplane * 2.0;
	const float factor = (znear / zplane);
	const float left   = (vppx - halfSX) * factor;
	const float bottom = (vppy - halfSY) * factor;
	const float right  = ((vppx + vpsx) - halfSX) * factor;
	const float top    = ((vppy + vpsy) - halfSY) * factor;
	glFrustum(left, right, bottom, top, znear, zfar);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	// translate so that (0,0,0) is on the zplane,
	// on the window's bottom left corner
	const float distAdj = (zplane / znear);
	glTranslatef(left * distAdj, bottom * distAdj, -zplane);

	// back light
	const float backLightPos[4]  = { 1.0f, 2.0f, 2.0f, 0.0f };
	const float backLightAmbt[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	const float backLightDiff[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
	const float backLightSpec[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, backLightPos);
	glLightfv(GL_LIGHT0, GL_AMBIENT,  backLightAmbt);
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  backLightDiff);
	glLightfv(GL_LIGHT0, GL_SPECULAR, backLightSpec);

	// sun light -- needs the camera transformation
	glPushMatrix();
	glLoadMatrixd(camera->modelview);
	glLightfv(GL_LIGHT1, GL_POSITION, gs->sunVector4);

	const float sunFactor = 1.0f;
	const float sf = sunFactor;
	const float* la = LightAmbientLand;
	const float* ld = LightDiffuseLand;

	const float sunLightAmbt[4] = { la[0]*sf, la[1]*sf, la[2]*sf, la[3]*sf };
	const float sunLightDiff[4] = { ld[0]*sf, ld[1]*sf, ld[2]*sf, ld[3]*sf };
	const float sunLightSpec[4] = { la[0]*sf, la[1]*sf, la[2]*sf, la[3]*sf };
	glLightfv(GL_LIGHT1, GL_AMBIENT,  sunLightAmbt);
	glLightfv(GL_LIGHT1, GL_DIFFUSE,  sunLightDiff);
	glLightfv(GL_LIGHT1, GL_SPECULAR, sunLightSpec);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);
	glPopMatrix();

	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);

	const float grey[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
	const float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, black);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, grey);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, black);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, black);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0);

	glEnable(GL_NORMALIZE);
}


static void RevertScreenTransform()
{
	glDisable(GL_NORMALIZE);

	glDisable(GL_LIGHT1);
	glDisable(GL_LIGHT0);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}


bool CLuaUI::DrawScreenItems()
{
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "DrawScreenItems");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return true; // the call is not defined
	}

	drawingEnabled = true;

	// push the current GL state
	glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT |
	             GL_POLYGON_BIT | GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT |
	             GL_LINE_BIT | GL_POINT_BIT);

	// setup a known state
	ResetGLState();

	// setup the screen transformation
	SetupScreenTransform();

	matrixDepth = 0;

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	lua_pushnumber(L, gu->viewSizeX);	
	lua_pushnumber(L, gu->viewSizeY);	

	// call the routine
	bool retval = true;
	const int error = lua_pcall(L, 2, 0, 0);
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n", error,
		                "Call_DrawScreenItems", lua_tostring(L, -1));
		lua_pop(L, 1);
		retval = false;
	}
	
	if (matrixDepth != 0) {
		logOutput.Print("DrawScreenItems: matrix mismatch");
		while (matrixDepth > 0) {
			glPopMatrix();
			matrixDepth--;
		}
	}

	// revert the screen transform
	RevertScreenTransform();

	// pop the GL state
	glPopAttrib();

	drawingEnabled = false;

	return retval;
}


bool CLuaUI::KeyPress(unsigned short key, bool isRepeat)
{
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "KeyPress");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, key);

	lua_newtable(L);
	lua_pushstring(L, "alt");
	lua_pushboolean(L, keys[SDLK_LALT]);
	lua_rawset(L, -3);
	lua_pushstring(L, "ctrl");
	lua_pushboolean(L, keys[SDLK_LCTRL]);
	lua_rawset(L, -3);
	lua_pushstring(L, "meta");
	lua_pushboolean(L, keys[SDLK_LMETA]);
	lua_rawset(L, -3);
	lua_pushstring(L, "shift");
	lua_pushboolean(L, keys[SDLK_LSHIFT]);
	lua_rawset(L, -3);

	lua_pushboolean(L, isRepeat);

	CKeySet ks(key, false);
	lua_pushstring(L, ks.GetString(true).c_str());

	// call the function
	const int error = lua_pcall(L, 4, 1, 0);
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n", error,
		                "Call_KeyPress", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}

	const int args = lua_gettop(L);
	if ((args == 1) && lua_isboolean(L, -1)) {
		return !!lua_toboolean(L, -1);
	}

	return false;
}


bool CLuaUI::KeyRelease(unsigned short key)
{
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "KeyRelease");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, key);

	lua_newtable(L);
	lua_pushstring(L, "alt");
	lua_pushboolean(L, keys[SDLK_LALT]);
	lua_rawset(L, -3);
	lua_pushstring(L, "ctrl");
	lua_pushboolean(L, keys[SDLK_LCTRL]);
	lua_rawset(L, -3);
	lua_pushstring(L, "meta");
	lua_pushboolean(L, keys[SDLK_LMETA]);
	lua_rawset(L, -3);
	lua_pushstring(L, "shift");
	lua_pushboolean(L, keys[SDLK_LSHIFT]);
	lua_rawset(L, -3);

	CKeySet ks(key, false);
	lua_pushstring(L, ks.GetString(true).c_str());

	// call the function
	const int error = lua_pcall(L, 3, 1, 0);
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n", error,
		                "Call_KeyRelease", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}

	const int args = lua_gettop(L);
	if ((args == 1) && lua_isboolean(L, -1)) {
		return !!lua_toboolean(L, -1);
	}

	return false;
}


bool CLuaUI::MouseMove(int x, int y, int dx, int dy, int button)
{
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "MouseMove");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, x - gu->viewPosX);
	lua_pushnumber(L, gu->viewSizeY - y - 1);
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


bool CLuaUI::MousePress(int x, int y, int button)
{
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "MousePress");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, x - gu->viewPosX);
	lua_pushnumber(L, gu->viewSizeY - y - 1);
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


int CLuaUI::MouseRelease(int x, int y, int button)
{
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "MouseRelease");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return false; // the call is not defined, do not take the event
	}

	lua_pushnumber(L, x - gu->viewPosX);
	lua_pushnumber(L, gu->viewSizeY - y - 1);
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
	if ((args == 1) && lua_isnumber(L, -1)) {
		return (int)lua_tonumber(L, -1);
	}

	return -1;
}


bool CLuaUI::IsAbove(int x, int y)
{
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "IsAbove");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return false; // the call is not defined
	}

	lua_pushnumber(L, x - gu->viewPosX);
	lua_pushnumber(L, gu->viewSizeY - y - 1);

	// call the function
	const int error = lua_pcall(L, 2, 1, 0);
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n", error,
		                "Call_IsAbove", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}

	const int args = lua_gettop(L);
	if ((args == 1) && lua_isboolean(L, -1)) {
		return !!lua_toboolean(L, -1);
	}

	return false;
}


string CLuaUI::GetTooltip(int x, int y)
{
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return "";
	}
	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "GetTooltip");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return ""; // the call is not defined
	}

	lua_pushnumber(L, x - gu->viewPosX);
	lua_pushnumber(L, gu->viewSizeY - y - 1);

	// call the function
	const int error = lua_pcall(L, 2, 1, 0);
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n", error,
		                "Call_GetTooltip", lua_tostring(L, -1));
		lua_pop(L, 1);
		return "";
	}

	const int args = lua_gettop(L);
	if ((args == 1) && lua_isstring(L, 1)) {
		return string(lua_tostring(L, 1));
	}

	return "";
}


bool CLuaUI::AddConsoleLine(const string& line, int priority)
{
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "AddConsoleLine");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return true; // the call is not defined
	}

	lua_pushstring(L, line.c_str());
	lua_pushnumber(L, priority);

	// call the function
	const int error = lua_pcall(L, 2, 0, 0);
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n", error,
		                "Call_AddConsoleLine", lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}

	return true;
}


bool CLuaUI::HasLayoutButtons()
{
	lua_State* L = LUASTATE.GetL();
	if (L == NULL) {
		return false;
	}
	lua_pop(L, lua_gettop(L));

	lua_getglobal(L, "LayoutButtons");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, lua_gettop(L));
		return false;
	}
	lua_pop(L, lua_gettop(L));
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
	buttonList.clear();
	menuName = "";

	lua_getglobal(L, "LayoutButtons");
	if (!lua_isfunction(L, -1)) {
		logOutput.Print("Error: missing LayoutButtons() lua function\n");
		lua_pop(L, lua_gettop(L));
		return false;
	}

	lua_pushnumber(L, xButtons);
	lua_pushnumber(L, yButtons);
	lua_pushnumber(L, cmds.size());

	if (!BuildCmdDescTable(L, cmds)) {
		lua_pop(L, lua_gettop(L));
		return false;
	}

	// call the function
	const int error = lua_pcall(L, 4, LUA_MULTRET, 0);
	if (error != 0) {
		logOutput.Print("error = %i, %s, %s\n", error,
		                "Call_LayoutButtons", lua_tostring(L, -1));
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
		lua_pop(L, lua_gettop(L));
		return false;
	}

	if (!GetLuaCmdDescList(L, 5, customCmds)) {
		logOutput.Print("LayoutButtons() bad customCommands table\n");
		lua_pop(L, lua_gettop(L));
		return false;
	}

	if (!GetLuaIntList(L, 6, onlyTextureCmds)) {
		logOutput.Print("LayoutButtons() bad onlyTexture table\n");
		lua_pop(L, lua_gettop(L));
		return false;
	}

	if (!GetLuaReStringList(L, 7, reTextureCmds)) {
		logOutput.Print("LayoutButtons() bad reTexture table\n");
		lua_pop(L, lua_gettop(L));
		return false;
	}

	if (!GetLuaReStringList(L, 8, reNamedCmds)) {
		logOutput.Print("LayoutButtons() bad reNamed table\n");
		lua_pop(L, lua_gettop(L));
		return false;
	}

	if (!GetLuaReStringList(L, 9, reTooltipCmds)) {
		logOutput.Print("LayoutButtons() bad reTooltip table\n");
		lua_pop(L, lua_gettop(L));
		return false;
	}

	if (!GetLuaReParamsList(L, 10, reParamsCmds)) {
		logOutput.Print("LayoutButtons() bad reParams table\n");
		lua_pop(L, lua_gettop(L));
		return false;
	}

	if (!GetLuaIntMap(L, 11, buttonList)) {
		logOutput.Print("LayoutButtons() bad buttonList table\n");
		lua_pop(L, lua_gettop(L));
		return false;
	}

	return true;
}


bool CLuaUI::BuildCmdDescTable(lua_State* L,
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


bool CLuaUI::GetLuaIntMap(lua_State* L, int index, map<int, int>& intMap)
{
	const int table = index;
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

	return true;
}


bool CLuaUI::GetLuaIntList(lua_State* L, int index, vector<int>& intList)
{
	const int table = index;
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

	return true;
}


bool CLuaUI::GetLuaReStringList(lua_State* L, int index,
                                vector<ReStringPair>& reStringList)
{
	const int table = index;
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

	return true;
}


bool CLuaUI::GetLuaReParamsList(lua_State* L, int index,
                                vector<ReParamsPair>& reParamsCmds)
{
	const int table = index;
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

	return true;
}


bool CLuaUI::GetLuaCmdDescList(lua_State* L, int index,
                               vector<CommandDescription>& cmdDescs)
{
	const int table = index;
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
		lua_pushstring(L, "Incorrect arguments to LoadTextVFS(\"filename\")");
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


static int GetDirListVFS(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isstring(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to GetDirListVFS(\"dir\")");
		lua_error(L);
	}
	const string dir = lua_tostring(L, 1);
	vector<string> filenames = hpiHandler->GetFilesInDir(dir);

	lua_newtable(L);
	for (int i = 0; i < filenames.size(); i++) {
		lua_pushnumber(L, i + 1);
		lua_pushstring(L, filenames[i].c_str());
		lua_rawset(L, -3);
	}
	lua_pushstring(L, "n");
	lua_pushnumber(L, filenames.size());
	lua_rawset(L, -3);

	return 1;
}


static int GetDirList(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1) ||
	    ((args >= 2) && !lua_isstring(L, 2)) ||
	    ((args >= 3) && !lua_isstring(L, 3))) {
		lua_pushstring(L, "Incorrect arguments to "
			"GetDirList(\"dir\" [,\"pattern\" [,\"opts\"]])");
		lua_error(L);
	}

	// keep searches within the Spring directory
	const string dir = lua_tostring(L, 1);
	if ((dir[0] == '/') || (dir[0] == '\\') ||
	    (strstr(dir.c_str(), "..") != NULL) ||
	    ((dir.size() > 0) && (dir[1] == ':'))) {
		const string errmsg = "Invalid GetDirList() access: " + dir;
		lua_pushstring(L, errmsg.c_str());
		lua_error(L);
	}

	string pat = "*";
	if (args >= 2) {
		pat = lua_tostring(L, 2);
		if (pat.empty()) {
			pat = "*"; // FindFiles() croaks on empty strings
		}
	}

	int opts = 0;
	if (args >= 3) {
		const string optstr = lua_tostring(L, 3);
		if (strstr(optstr.c_str(), "r") != NULL) {
			opts |= FileSystem::RECURSE;
		}
		if (strstr(optstr.c_str(), "d") != NULL) {
			opts |= FileSystem::INCLUDE_DIRS;
		}
	}

	vector<string> filenames = filesystem.FindFiles(dir, pat, opts);

	lua_newtable(L);
	for (int i = 0; i < filenames.size(); i++) {
		lua_pushnumber(L, i + 1);
		lua_pushstring(L, filenames[i].c_str());
		lua_rawset(L, -3);
	}
	lua_pushstring(L, "n");
	lua_pushnumber(L, filenames.size());
	lua_rawset(L, -3);

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
	if (guihandler != NULL) {
		guihandler->RunCustomCommands(cmds, false);
	}
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


static int GetLastFrameSeconds(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetLastFrameSeconds() takes no arguments");
		lua_error(L);
	}
	lua_pushnumber(L, gu->lastFrameTime);
	return 1;
}


static int GetActiveCommand(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetActiveCommand() takes no arguments");
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


static int GetDefaultCommand(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetDefaultCommand() takes no arguments");
		lua_error(L);
	}
	if (guihandler != NULL) {
		const int defCmd = guihandler->GetDefaultCommand(mouse->lastx, mouse->lasty);
		lua_pushnumber(L, defCmd);
		if ((defCmd < 0) && (defCmd >= guihandler->commands.size())) {
			return 1;
		} else {
			lua_pushnumber(L, guihandler->commands[defCmd].id);
			lua_pushnumber(L, guihandler->commands[defCmd].type);
			lua_pushstring(L, guihandler->commands[defCmd].name.c_str());
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
	lua_pushnumber(L, mouse->lastx - gu->viewPosX);
	lua_pushnumber(L, gu->viewSizeY - mouse->lasty - 1);
	lua_pushboolean(L, mouse->buttons[SDL_BUTTON_LEFT].pressed);
	lua_pushboolean(L, mouse->buttons[SDL_BUTTON_MIDDLE].pressed);
	lua_pushboolean(L, mouse->buttons[SDL_BUTTON_RIGHT].pressed);
	return 5;
}


static int GetMouseCursor(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetMouseCursor() takes no arguments");
		lua_error(L);
	}
	lua_pushstring(L, mouse->cursorText.c_str());
	lua_pushnumber(L, mouse->cursorScale);
	return 2;
}


static int SetMouseCursor(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1) ||
	    ((args >= 2) && !lua_isnumber(L, 2))) {
		lua_pushstring(L,
			"Incorrect arguments to SetMouseCursor(\"name\", [scale])");
		lua_error(L);
	}
	mouse->cursorText = lua_tostring(L, 1);
	if (args >= 2) {
		mouse->cursorScale = lua_tonumber(L, 2);
	}
	return 0;
}


static int GetKeyState(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to GetKeyState(keycode)");
		lua_error(L);
	}
	const int key = (int)lua_tonumber(L, 1);
	if ((key < 0) || (key >= SDLK_LAST)) {
		lua_pushboolean(L, 0);
	} else {
		lua_pushboolean(L, keys[key]);
	}
	return 1;
}


static int GetModKeyState(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetModKeyState() takes no arguments");
		lua_error(L);
	}
	lua_pushboolean(L, keys[SDLK_LALT]);
	lua_pushboolean(L, keys[SDLK_LCTRL]);
	lua_pushboolean(L, keys[SDLK_LMETA]);
	lua_pushboolean(L, keys[SDLK_LSHIFT]);
	return 4;
}


static int GetPressedKeys(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetPressedKeys() takes no arguments");
		lua_error(L);
	}
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



static int GetCurrentTooltip(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetCurrentTooltip() takes no arguments");
		lua_error(L);
	}
	const string tooltip = mouse->GetCurrentTooltip();
	lua_pushstring(L, tooltip.c_str());
	return 1;
}


static int GetKeyCode(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isstring(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to GetKeyCode(\"keysym\")");
		lua_error(L);
	}
	const string keysym = lua_tostring(L, 1);
	lua_pushnumber(L, keyCodes->GetCode(keysym));
	return 1;
}


static int GetKeySymbol(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to GetKeySymbol(keycode)");
		lua_error(L);
	}
	const int keycode = (int)lua_tonumber(L, 1);
	lua_pushstring(L, keyCodes->GetName(keycode).c_str());
	lua_pushstring(L, keyCodes->GetDefaultName(keycode).c_str());
	return 2;
}


static int GetKeyBindings(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isstring(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to GetKeyBindings(\"keyset\")");
		lua_error(L);
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


static int GetActionHotKeys(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isstring(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to GetActionHotKeys(\"command\")");
		lua_error(L);
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

static int SetUnitDefIcon(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
		lua_pushstring(L,
			"Incorrect arguments to SetUnitDefIcon(unitDefID, \"icon\")");
		lua_error(L);
	}
	const int unitDefID = (int)lua_tonumber(L, 1);
	UnitDef* ud = unitDefHandler->GetUnitByID(unitDefID);
	if (ud == NULL) {
		return 0;
	}

	ud->iconType = lua_tostring(L, 2);

	if (commanderDefs.find(ud) != commanderDefs.end()) {
		// set all commander-like unitdefs to the same icon
		// so that decoy commanders can not be distinguished
		set<UnitDef*>::iterator cit;
		for (cit = commanderDefs.begin(); cit != commanderDefs.end(); ++cit) {
			(*cit)->iconType = ud->iconType;
		}
	}

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


static int GetGroupAIList(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetGroupAIList() takes no arguments");
		lua_error(L);
	}
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


static int GetGroupAIName(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to GetGroupAIName(groupID)");
		lua_error(L);
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


/******************************************************************************/

enum UnitExtraParam {
	UnitDefId,
	UnitGroupId,
	UnitTeamId
};


static void PackUnitsUnsorted(lua_State* L, const set<CUnit*>& unitSet,
                              UnitExtraParam extraParam)
{

	lua_newtable(L);
	int count = 0;
	set<CUnit*>::const_iterator uit;
	for (uit = unitSet.begin(); uit != unitSet.end(); ++uit) {
		count++;
		lua_pushnumber(L, count);
		lua_pushnumber(L, (*uit)->id);
		lua_rawset(L, -3);
	}
	lua_pushstring(L, "n");
	lua_pushnumber(L, count);
	lua_rawset(L, -3);
}


static void PackUnitsCounts(lua_State* L, const set<CUnit*>& unitSet)
{
	map<int, int> countMap;
	set<CUnit*>::const_iterator uit;
	for (uit = unitSet.begin(); uit != unitSet.end(); ++uit) {
		CUnit* unit = *uit;
		if (unit->unitDef == NULL) {
			continue;
		}
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
	lua_pushstring(L, "n");
	lua_pushnumber(L, countMap.size());
	lua_rawset(L, -3);
}


static void PackUnitsSorted(lua_State* L, const set<CUnit*>& unitSet,
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
}


static int GetSelectedUnits(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetSelectedUnits() takes no arguments");
		lua_error(L);
	}
	PackUnitsUnsorted(L, selectedUnits.selectedUnits, UnitGroupId);
	return 1;
}


static int GetSelectedUnitsSorted(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetSelectedUnitsSorted() takes no arguments");
		lua_error(L);
	}
	PackUnitsSorted(L, selectedUnits.selectedUnits, UnitGroupId);
	return 1;
}


static int GetSelectedUnitsCounts(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetSelectedUnitsCounts() takes no arguments");
		lua_error(L);
	}
	PackUnitsCounts(L, selectedUnits.selectedUnits);
	return 1;
}


static int GetGroupUnits(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to GetGroupUnits(groupID)");
		lua_error(L);
	}
	const int groupID = (int)lua_tonumber(L, 1);
	const vector<CGroup*>& groups = grouphandler->groups;
	if ((groupID < 0) || (groupID >= groups.size()) ||
	    (groups[groupID] == NULL)) {
		return 0; // nils
	}
	PackUnitsUnsorted(L, groups[groupID]->units, UnitDefId);
	return 1;
}


static int GetGroupUnitsSorted(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to GetGroupUnitsSorted(groupID)");
		lua_error(L);
	}
	const int groupID = (int)lua_tonumber(L, 1);
	const vector<CGroup*>& groups = grouphandler->groups;
	if ((groupID < 0) || (groupID >= groups.size()) ||
	    (groups[groupID] == NULL)) {
		return 0; // nils
	}
	PackUnitsSorted(L, groups[groupID]->units, UnitDefId);
	return 1;
}


static int GetGroupUnitsCounts(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetGroupUnitsCounts() takes no arguments");
		lua_error(L);
	}
	const int groupID = (int)lua_tonumber(L, 1);
	const vector<CGroup*>& groups = grouphandler->groups;
	if ((groupID < 0) || (groupID >= groups.size()) ||
	    (groups[groupID] == NULL)) {
		return 0; // nils
	}
	PackUnitsCounts(L, groups[groupID]->units);
	return 1;
}


static int GetTeamUnits(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to GetTeamUnits(teamID)");
		lua_error(L);
	}
	const int teamID = (int)lua_tonumber(L, 1);
	if ((teamID < 0) || (teamID >= MAX_TEAMS)) {
		return 0;
	}
	const CTeam* team = gs->Team(teamID);
	if ((team == NULL) || !gs->AlliedTeams(gu->myTeam, teamID)) {
		return 0;
	}
	PackUnitsUnsorted(L, gs->Team(teamID)->units, UnitDefId);
	return 1;
}


static int GetTeamUnitsSorted(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to GetTeamUnitsSorted(teamID)");
		lua_error(L);
	}
	const int teamID = (int)lua_tonumber(L, 1);
	if ((teamID < 0) || (teamID >= MAX_TEAMS)) {
		return 0;
	}
	const CTeam* team = gs->Team(teamID);
	if ((team == NULL) || !gs->AlliedTeams(gu->myTeam, teamID)) {
		return 0;
	}
	PackUnitsSorted(L, gs->Team(teamID)->units, UnitDefId);
	return 1;
}


static int GetTeamUnitsCounts(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to GetTeamUnitsCounts(teamID)");
		lua_error(L);
	}
	const int teamID = (int)lua_tonumber(L, 1);
	if ((teamID < 0) || (teamID >= MAX_TEAMS)) {
		return 0;
	}
	const CTeam* team = gs->Team(teamID);
	if ((team == NULL) || !gs->AlliedTeams(gu->myTeam, teamID)) {
		return 0;
	}

	lua_newtable(L);
	int count = 0;
	for (int udID = 0; udID < (unitDefHandler->numUnits + 1); udID++) {
		const int unitCount = uh->unitsType[teamID][udID];
		if (unitCount > 0) {
			lua_pushnumber(L, udID);
			lua_pushnumber(L, unitCount);
			lua_rawset(L, -3);
			count++;
		}
	}
	lua_pushstring(L, "n");
	lua_pushnumber(L, count);
	lua_rawset(L, -3);

	return 1; // FIXME -- implement
}


/******************************************************************************/

static int GetUnitsInBox(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 6) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) ||
	    !lua_isnumber(L, 4) || !lua_isnumber(L, 5) || !lua_isnumber(L, 6)) {
		lua_pushstring(L,
			"Incorrect arguments to GetUnitsInBox(minx,miny,minz, maxx,maxy,maxz");
		lua_error(L);
	}
	const float minx = (float)lua_tonumber(L, 1);
	const float miny = (float)lua_tonumber(L, 2);
	const float minz = (float)lua_tonumber(L, 3);
	const float maxx = (float)lua_tonumber(L, 4);
	const float maxy = (float)lua_tonumber(L, 5);
	const float maxz = (float)lua_tonumber(L, 6);

	lua_newtable(L);
	int count = 0;
	const set<CUnit*>& teamUnits = gs->Team(gu->myTeam)->units;
	set<CUnit*>::const_iterator it;
	for (it = teamUnits.begin(); it != teamUnits.end(); ++it) {
		const CUnit* unit = (*it);
		const float r = unit->radius;
		const float3& p = unit->midPos;
		if (((p.x + r) < minx) || ((p.y + r) < miny) || ((p.z + r) < minz) ||
		    ((p.x - r) > maxx) || ((p.y - r) > maxy) || ((p.z - r) > maxz)) {
			continue;
		}
		count++;
		lua_pushnumber(L, count);
		lua_pushnumber(L, unit->id);
		lua_rawset(L, -3);
	}
	lua_pushstring(L, "n");
	lua_pushnumber(L, count);
	lua_rawset(L, -3);
	return 1;
}


static int GetUnitsInCylinder(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 3) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
		lua_pushstring(L, "Incorrect arguments to GetUnitsInCylinder(x,z,r)");
		lua_error(L);
	}
	const float x = (float)lua_tonumber(L, 1);
	const float z = (float)lua_tonumber(L, 2);
	const float radius = (float)lua_tonumber(L, 3);

	lua_newtable(L);
	int count = 0;
	const set<CUnit*>& teamUnits = gs->Team(gu->myTeam)->units;
	set<CUnit*>::const_iterator it;
	for (it = teamUnits.begin(); it != teamUnits.end(); ++it) {
		const CUnit* unit = (*it);
		const float3& pos = unit->midPos;
		const float dx = x - pos.x;
		const float dz = z - pos.z;
		const float dist = sqrtf((dx * dx) + (dz * dz));
		if ((dist - unit->radius) > radius) {
			continue;
		}
		count++;
		lua_pushnumber(L, count);
		lua_pushnumber(L, unit->id);
		lua_rawset(L, -3);
	}
	lua_pushstring(L, "n");
	lua_pushnumber(L, count);
	lua_rawset(L, -3);
	return 1;
}


typedef struct Plane {
	float x, y, z, d;  // ax + by + cz + d = 0
};

static int GetUnitsInPlanes(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_istable(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to GetUnitsInPlanes()");
		lua_error(L);
	}

	vector<Plane> planes;
	const int table = lua_gettop(L);
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (lua_istable(L, -1)) {
			float values[4];
			const int v = ParseFloatArray(L, values, 4);
			if (v == 4) {
				Plane plane = { values[0], values[1], values[2], values[3] };
				planes.push_back(plane);
			}
		}
	}

	lua_newtable(L);
	int count = 0;
	const set<CUnit*>& teamUnits = gs->Team(gu->myTeam)->units;
	set<CUnit*>::const_iterator it;
	for (it = teamUnits.begin(); it != teamUnits.end(); ++it) {
		const CUnit* unit = (*it);
		const float3& pos = unit->midPos;
		int i;
		for (i = 0; i < (int)planes.size(); i++) {
			const Plane& p = planes[i];
			const float dist = (pos.x * p.x) + (pos.y * p.y) + (pos.z * p.z) + p.d;
			if ((dist - unit->radius) > 0.0f) {
				break; // outside
			}
		}
		if (i != (int)planes.size()) {
			continue;
		}
		count++;
		lua_pushnumber(L, count);
		lua_pushnumber(L, unit->id);
		lua_rawset(L, -3);
	}
	lua_pushstring(L, "n");
	lua_pushnumber(L, count);
	lua_rawset(L, -3);
	return 1;
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
	lua_pushstring(L, "repeating");
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


static int GetUnitResources(lua_State* L)
{
	CUnit* unit = AlliedUnit(L, __FUNCTION__);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->metalMake);
	lua_pushnumber(L, unit->metalUse);
	lua_pushnumber(L, unit->energyMake);
	lua_pushnumber(L, unit->energyUse);
	return 4;
}


static int GetUnitExperience(lua_State* L)
{
	CUnit* unit = AlliedUnit(L, __FUNCTION__);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->experience);
	return 1;
}


static int GetUnitRadius(lua_State* L)
{
	CUnit* unit = AlliedUnit(L, __FUNCTION__);
	if (unit == NULL) {
		return 0;
	}
	lua_pushnumber(L, unit->radius);
	return 1;
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


static int GetUnitDefDimensions(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to GetUnitDefDimensions()");
		lua_error(L);
	}
	const int unitDefID = (int)lua_tonumber(L, 1);
	const UnitDef* ud = unitDefHandler->GetUnitByID(unitDefID);
	if (ud == NULL) {
		return 0;
	}
	const S3DOModel* model;
	model = modelParser->Load3DO(ud->model.modelpath, 1.0f, gu->myTeam);
	if (model == NULL) {
		return 0;
	}
	const S3DOModel& m = *model;
	const float3& mid = model->relMidPos;
	lua_newtable(L);
	lua_pushstring(L, "height"); lua_pushnumber(L, m.height); lua_rawset(L, -3);
	lua_pushstring(L, "radius"); lua_pushnumber(L, m.radius); lua_rawset(L, -3);
	lua_pushstring(L, "midx");   lua_pushnumber(L, mid.x);    lua_rawset(L, -3);
	lua_pushstring(L, "minx");   lua_pushnumber(L, m.minx);   lua_rawset(L, -3);
	lua_pushstring(L, "maxx");   lua_pushnumber(L, m.maxx);   lua_rawset(L, -3);
	lua_pushstring(L, "midy");   lua_pushnumber(L, mid.y);    lua_rawset(L, -3);
	lua_pushstring(L, "miny");   lua_pushnumber(L, m.miny);   lua_rawset(L, -3);
	lua_pushstring(L, "maxy");   lua_pushnumber(L, m.maxy);   lua_rawset(L, -3);
	lua_pushstring(L, "midz");   lua_pushnumber(L, mid.z);    lua_rawset(L, -3);
	lua_pushstring(L, "minz");   lua_pushnumber(L, m.minz);   lua_rawset(L, -3);
	lua_pushstring(L, "maxz");   lua_pushnumber(L, m.maxz);   lua_rawset(L, -3);
	return 1;
}


static int PackCommandQueue(lua_State* L, const deque<Command>& q, int count)
{
	lua_newtable(L);

	int i = 0;
	deque<Command>::const_iterator it;
	for (it = q.begin(); it != q.end(); it++) {
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


static int GetCommandQueue(lua_State* L)
{
	CUnit* unit = AlliedUnit(L, __FUNCTION__);
	if (unit == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args >= 2) && !lua_isnumber(L, 2)) {
		lua_pushstring(L,
			"Incorrect arguments to GetCommandQueue(unitID [, count])");
		lua_error(L);
	}
	const CCommandAI* commandAI = unit->commandAI;
	if (commandAI == NULL) {
		return 0;
	}

	// send the new unit commands for factories, otherwise the normal commands
	const deque<Command>* queue;
	const CFactoryCAI* factoryCAI = dynamic_cast<const CFactoryCAI*>(commandAI);
	if (factoryCAI == NULL) {
		queue = &commandAI->commandQue;
	} else {
		queue = &factoryCAI->newUnitCommands;
	}

	// get the desired number of commands to return
	int count = -1;
	if (args > 1) {
 		count = (int)lua_tonumber(L, 2);
	}
	if (count < 0) {
		count = (int)queue->size();
	}

	PackCommandQueue(L, *queue, count);

	return 1;
}


static int PackBuildQueue(lua_State* L, bool canBuild, const char* caller)
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
	return PackBuildQueue(L, false, __FUNCTION__);
}


static int GetRealBuildQueue(lua_State* L)
{
	return PackBuildQueue(L, true, __FUNCTION__);
}


/******************************************************************************/

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


static int GetMyTeamID(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetMyTeamID() takes no arguments");
		lua_error(L);
	}
	lua_pushnumber(L, gu->myTeam);
	return 1;
}


static int GetMyAllyTeamID(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetMyAllyTeamID() takes no arguments");
		lua_error(L);
	}
	lua_pushnumber(L, gu->myAllyTeam);
	return 1;
}


/******************************************************************************/

static int GetAllyTeamList(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetAllyTeamList() takes no arguments");
		lua_error(L);
	}

	lua_newtable(L);
	int count = 0;
	for (int at = 0; at < gs->activeAllyTeams; at++) {
		count++;
		lua_pushnumber(L, count);
		lua_pushnumber(L, at);
		lua_rawset(L, -3);
	}
	lua_pushstring(L, "n");
	lua_pushnumber(L, count);
	lua_rawset(L, -3);

	return 1;
}


static int GetTeamList(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 0) && ((args != 1) || !lua_isnumber(L, 1))) {
		lua_pushstring(L, "Incorrect arguments to GetTeamList([allyTeamID])");
		lua_error(L);
	}
	
	int allyTeamID = -1;
	if (args == 1) {
		allyTeamID = (int)lua_tonumber(L, 1);
		if ((allyTeamID < 0) || (allyTeamID >= gs->activeAllyTeams)) {
			return 0;
		}
	}

	lua_newtable(L);
	int count = 0;
	for (int t = 0; t < gs->activeTeams; t++) {
		if (gs->Team(t) == NULL) {
			continue;
		}
		if ((allyTeamID < 0) || (allyTeamID == gs->AllyTeam(t))) {
			count++;
			lua_pushnumber(L, count);
			lua_pushnumber(L, t);
			lua_rawset(L, -3);
		}
	}
	lua_pushstring(L, "n");
	lua_pushnumber(L, count);
	lua_rawset(L, -3);
	
	return 1;
}


static int GetPlayerList(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 0) && ((args != 1) || !lua_isnumber(L, 1))) {
		lua_pushstring(L, "Incorrect arguments to GetPlayerList([teamID])");
		lua_error(L);
	}
	
	int teamID = -1;
	if (args == 1) {
		teamID = (int)lua_tonumber(L, 1);
		if ((teamID < 0) || (teamID >= MAX_TEAMS) || (gs->Team(teamID) == NULL)) {
			return 0;
		}
	}

	lua_newtable(L);
	int count = 0;
	for (int p = 0; p < gs->activePlayers; p++) {
		const CPlayer* player = gs->players[p];
		if (player == NULL) {
			continue;
		}
		if ((teamID < 0) || (player->team == teamID)) {
			count++;
			lua_pushnumber(L, count);
			lua_pushnumber(L, p);
			lua_rawset(L, -3);
		}
	}
	lua_pushstring(L, "n");
	lua_pushnumber(L, count);
	lua_rawset(L, -3);
	
	return 1;
}


/******************************************************************************/

static void AddPlayerToRoster(lua_State* L, int playerID)
{
	const CPlayer* p = gs->players[playerID];
	int index = 1;
	lua_newtable(L);
	lua_pushnumber(L, index); index++;
	lua_pushstring(L, p->playerName.c_str());
	lua_rawset(L, -3);
	lua_pushnumber(L, index); index++;
	lua_pushnumber(L, playerID);
	lua_rawset(L, -3);
	lua_pushnumber(L, index); index++;
	lua_pushnumber(L, p->team);
	lua_rawset(L, -3);
	lua_pushnumber(L, index); index++;
	lua_pushnumber(L, gs->AllyTeam(p->team));
	lua_rawset(L, -3);
	lua_pushnumber(L, index); index++;
	lua_pushboolean(L, p->spectator);
	lua_rawset(L, -3);
	lua_pushnumber(L, index); index++;
	lua_pushnumber(L, p->cpuUsage);
	lua_rawset(L, -3);
	lua_pushnumber(L, index); index++;
	lua_pushnumber(L, p->ping);
	lua_rawset(L, -3);
}


static int GetPlayerRoster(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 0) && ((args != 1) || !lua_isnumber(L, 1))) {
		lua_pushstring(L, "Incorrect arguments to GetPlayerRoster([type])");
		lua_error(L);
	}
	
	if (args == 1) {
		const int type = (int)lua_tonumber(L, 1);
		playerRoster.SetSortTypeByCode((PlayerRoster::SortType)type);
	} else {
		playerRoster.SetSortTypeByCode(PlayerRoster::Allies);
	}

	int count;
	const int* players = playerRoster.GetIndices(&count);

	lua_newtable(L);
	for (int i = 0; i < count; i++) {
		lua_pushnumber(L, i + 1);
		AddPlayerToRoster(L, players[i]);
		lua_rawset(L, -3);
	}
	lua_pushstring(L, "n");
	lua_pushnumber(L, count);
	lua_rawset(L, -3);

	return 1;	
}


/******************************************************************************/

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
	lua_pushnumber(L, (float)team->color[0] / 255.0f);
	lua_pushnumber(L, (float)team->color[1] / 255.0f);
	lua_pushnumber(L, (float)team->color[2] / 255.0f);
	lua_pushnumber(L, (float)team->color[3] / 255.0f);

	return 10;
}


static int GetTeamResources(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
		lua_pushstring(L, "Incorrect arguments to GetTeamResources(teamID, \"type\")");
		lua_error(L);
	}

	const int teamID = (int)lua_tonumber(L, 1);
	if ((teamID < 0) || (teamID >= MAX_TEAMS)) {
		return 0;
	}
	const CTeam* team = gs->Team(teamID);
	if ((team == NULL) || !gs->AlliedTeams(gu->myTeam, teamID)) {
		return 0;
	}
	const string type = lua_tostring(L, 2);

	if (type == "metal") {
		lua_pushnumber(L, team->metal);
		lua_pushnumber(L, team->metalStorage);
		lua_pushnumber(L, team->prevMetalPull);
		lua_pushnumber(L, team->prevMetalIncome);
		lua_pushnumber(L, team->prevMetalExpense);
		lua_pushnumber(L, team->metalShare);
		lua_pushnumber(L, team->metalSent);
		lua_pushnumber(L, team->metalReceived);
		return 8;
	}

	if (type == "energy") {
		lua_pushnumber(L, team->energy);
		lua_pushnumber(L, team->energyStorage);
		lua_pushnumber(L, team->prevEnergyPull);
		lua_pushnumber(L, team->prevEnergyIncome);
		lua_pushnumber(L, team->prevEnergyExpense);
		lua_pushnumber(L, team->energyShare);
		lua_pushnumber(L, team->energySent);
		lua_pushnumber(L, team->energyReceived);
		return 8;
	}

	return 0;
}


static int GetTeamUnitStats(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to GetTeamUnitStats(teamID)");
		lua_error(L);
	}

	const int teamID = (int)lua_tonumber(L, 1);
	if ((teamID < 0) || (teamID >= MAX_TEAMS)) {
		return 0;
	}
	const CTeam* team = gs->Team(teamID);
	if (team == NULL) {
		return 0;
	}

	lua_pushnumber(L, team->currentStats.unitsKilled);
	lua_pushnumber(L, team->currentStats.unitsDied);
	lua_pushnumber(L, team->currentStats.unitsCaptured);
	lua_pushnumber(L, team->currentStats.unitsOutCaptured);
	lua_pushnumber(L, team->currentStats.unitsReceived);
	lua_pushnumber(L, team->currentStats.unitsSent);

	return 6;
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


static int GetCameraPos(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetCameraPos() takes no arguments");
		lua_error(L);
	}
	lua_pushnumber(L, camera->pos.x);
	lua_pushnumber(L, camera->pos.y);
	lua_pushnumber(L, camera->pos.z);
	return 3;
}


static int GetCameraVectors(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "GetCameraVectors() takes no arguments");
		lua_error(L);
	}

	const CCamera* cam = camera;

#define PACK_CAMERA_VECTOR(n) \
	lua_pushstring(L, #n);      \
	lua_newtable(L);            \
	lua_pushnumber(L, 1); lua_pushnumber(L, cam-> n .x); lua_rawset(L, -3); \
	lua_pushnumber(L, 2); lua_pushnumber(L, cam-> n .y); lua_rawset(L, -3); \
	lua_pushnumber(L, 3); lua_pushnumber(L, cam-> n .z); lua_rawset(L, -3); \
	lua_rawset(L, -3)

	lua_newtable(L);
	PACK_CAMERA_VECTOR(forward);
	PACK_CAMERA_VECTOR(up);
	PACK_CAMERA_VECTOR(right);
	PACK_CAMERA_VECTOR(top);
	PACK_CAMERA_VECTOR(bottom);
	PACK_CAMERA_VECTOR(leftside);
	PACK_CAMERA_VECTOR(rightside);

	return 1;
}


static int WorldToScreenCoords(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 3) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
		lua_pushstring(L, "Incorrect arguments to WorldToScreenCoords(x,y,z)");
		lua_error(L);
	}
	const float3 worldPos(lua_tonumber(L, 1),
	                      lua_tonumber(L, 2),
	                      lua_tonumber(L, 3));
	const float3 winPos = camera->CalcWindowCoordinates(worldPos);
	lua_pushnumber(L, winPos.x);
	lua_pushnumber(L, winPos.y);
	lua_pushnumber(L, winPos.z);
	return 3;
}


static int TraceScreenRay(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isnumber(L, 2) ||
	    ((args >= 3) && !lua_isboolean(L, 3))) {
		lua_pushstring(L, "Incorrect arguments to TraceScreenRay()");
		lua_error(L);
	}

	// window coordinates	
	const int wx = (int)lua_tonumber(L, 1) - gu->viewPosX;
	const int wy = gu->viewSizeY - 1 - (int)lua_tonumber(L, 2);
	if ((wx < gu->viewPosX) || (wx >= (gu->viewSizeX + gu->viewPosX)) ||
	    (wy < gu->viewPosY) || (wy >= (gu->viewSizeY + gu->viewPosY))) {
		return 0;
	}

	CUnit* unit = NULL;
	CFeature* feature = NULL;
	const float range = gu->viewRange * 1.4f;
	const float3& pos = camera->pos;
	const float3 dir = camera->CalcPixelDir(wx, wy);

	const bool onlyCoords = ((args >= 3) && lua_toboolean(L, 3));
	
	const float udist = helper->GuiTraceRay(pos, dir, range, unit, 20.0f, true);
	const float fdist = helper->GuiTraceRayFeature(pos, dir, range,feature);

	const float badRange = (range - 300.0f);
	if ((udist > badRange) && (fdist > badRange) && (unit == NULL)) {
		return 0;
	}

	if (!onlyCoords) {
		if (udist > fdist) {
			unit = NULL;
		}

		if (unit != NULL) {
			lua_pushstring(L, "unit");
			lua_pushnumber(L, unit->id);
			return 2;
		}

		if (feature != NULL) {
			lua_pushstring(L, "feature");
			lua_pushnumber(L, feature->id);
			return 2;
		}
	}

	const float3 groundPos = pos + (dir * udist);
	lua_pushstring(L, "ground");
	lua_newtable(L);
	lua_pushnumber(L, 1); lua_pushnumber(L, groundPos.x); lua_rawset(L, -3);
	lua_pushnumber(L, 2); lua_pushnumber(L, groundPos.y); lua_rawset(L, -3);
	lua_pushnumber(L, 3); lua_pushnumber(L, groundPos.z); lua_rawset(L, -3);

	return 2;
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


static int GetGroundHeight(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isnumber(L, 1) || !lua_isnumber(L, -2)) {
		lua_pushstring(L, "Incorrect arguments to GetGroundHeight(x, y");
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


static int Pos2BuildPos(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 4) || !lua_isnumber(L, 1) ||
	    !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		lua_pushstring(L,
			"Incorrect arguments to Pos2BuildPos(unitDefID, x, y, z)");
		lua_error(L);
	}
	const int unitDefID = (int)lua_tonumber(L, 1);
	UnitDef* ud = unitDefHandler->GetUnitByID(unitDefID);
	if (ud == NULL) {
		return 0;
	}
	const float3 pos((float)lua_tonumber(L, 2),
	                 (float)lua_tonumber(L, 3),
	                 (float)lua_tonumber(L, 4));

	const float3 buildPos = helper->Pos2BuildPos(pos, ud);

	lua_pushnumber(L, buildPos.x);
	lua_pushnumber(L, buildPos.y);
	lua_pushnumber(L, buildPos.z);

	return 3;
}


/******************************************************************************/

static int SetShareLevel(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isstring(L, 1) || !lua_isnumber(L, 2)) {
		lua_pushstring(L, "Incorrect arguments to SetShareLevel(\"type\", level");
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


static int ShareResources(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isstring(L, 2) ||
	    ((args >= 3) && !lua_isnumber(L, 3))) {
		lua_pushstring(L, "Incorrect arguments to ShareResources()");
		lua_error(L);
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
		net->SendData<unsigned char, unsigned char, unsigned char, float, float>(
			NETMSG_SHARE, gu->myPlayerNum, teamID, 1, 0.0f, 0.0f);
		selectedUnits.ClearSelected();
	}
	else if (args >= 3) {
		const float amount = (float)lua_tonumber(L, 3);
		if (type == "metal") {
			net->SendData<unsigned char, unsigned char, unsigned char, float, float>(
				NETMSG_SHARE, gu->myPlayerNum, teamID, 0, amount, 0.0f);
		}
		else if (type == "energy") {
			net->SendData<unsigned char, unsigned char, unsigned char, float, float>(
				NETMSG_SHARE, gu->myPlayerNum, teamID, 0, 0.0f, amount);
		}
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

static int AddWorldIcon(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 4) ||
	    !lua_isnumber(L, 1) || !lua_isnumber(L, 2) ||
	    !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		lua_pushstring(L, "Incorrect arguments to AddWorldIcon(id, x, y, z");
		lua_error(L);
	}
	const int cmdID = (int)lua_tonumber(L, 1);
	const float3 pos(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
	cursorIcons.AddIcon(cmdID, pos);
	return 0;
}


static int AddWorldText(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 4) ||
	    !lua_isstring(L, 1) || !lua_isnumber(L, 2) ||
	    !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
		lua_pushstring(L, "Incorrect arguments to AddWorldIcon(text, x, y, z");
		lua_error(L);
	}
	const string text = lua_tostring(L, 1);
	const float3 pos(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
	cursorIcons.AddIconText(text, pos);
	return 0;
}


static int AddWorldUnit(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 6) ||
	    !lua_isstring(L, 1) || !lua_isnumber(L, 2) ||
	    !lua_isnumber(L, 3) || !lua_isnumber(L, 4) ||
	    !lua_isnumber(L, 5) || !lua_isnumber(L, 6)) {
		lua_pushstring(L,
			"Incorrect arguments to AddWorldUnit(unitDefID, x, y, z, team, facing");
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
			"DrawText(msg, x, y [,size] [,\"options\"]");
		lua_error(L);
	}
	const string text = lua_tostring(L, 1);
	const float x     = lua_tonumber(L, 2);
	const float y     = lua_tonumber(L, 3);

	float size = 12.0f;
	bool right = false;
	bool center = false;
	bool outline = false;
	bool colorCodes = true;
	bool lightOut;

	if ((args >= 4) && lua_isnumber(L, 4)) {
		size = lua_tonumber(L, 4);
	}
	if ((args >= 5) && lua_isstring(L, 5)) {
	  const char* c = lua_tostring(L, 5);
	  while (*c != 0) {
	  	switch (*c) {
	  	  case 'c': { center = true;                    break; }
	  	  case 'r': { right = true;                     break; }
			  case 'n': { colorCodes = false;               break; }
			  case 'o': { outline = true; lightOut = false; break; }
			  case 'O': { outline = true; lightOut = true;  break; }
			}
	  	c++;
		}
	}

	const float yScale = size / textHeight;
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
		if (colorCodes) {
			font->glPrintColor("%s", text.c_str());
		} else {
			font->glPrint("%s", text.c_str());
		}
	} else {
		const float darkOutline[4]  = { 0.25f, 0.25f, 0.25f, 0.8f };
		const float lightOutline[4] = { 0.85f, 0.85f, 0.85f, 0.8f };
		const float noshow[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		const float xPixel = 1.0f / xScale;
		const float yPixel = 1.0f / yScale;
		if (lightOut) {
			font->glPrintOutlined(text.c_str(), xPixel, yPixel, noshow, lightOutline);
		} else {
			font->glPrintOutlined(text.c_str(), xPixel, yPixel, noshow, darkOutline);
		}
		if (colorCodes) {
			font->glPrintColor("%s", text.c_str());
		} else {
			font->glPrint("%s", text.c_str());
		}
	}
	glPopMatrix();

	return 0;	
}


static int DrawGetTextWidth(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isstring(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to DrawGetTextWidth(\"text\")");
		lua_error(L);
	}
	const string text = lua_tostring(L, 1);
	const float width = font->CalcTextWidth(text.c_str()) / textHeight;
	lua_pushnumber(L, width);
	return 1;
}


/******************************************************************************/

static int DrawUnitDef(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args < 2) || !lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
		lua_pushstring(L, "Incorrect arguments to DrawUnitDef(unitDefID, team)");
		lua_error(L);
	}
	const int unitDefID = (int)lua_tonumber(L, 1);
	const UnitDef* ud = unitDefHandler->GetUnitByID(unitDefID);
	if (ud == NULL) {
		return 0;
	}
	const int teamID = (int)lua_tonumber(L, 2);
	if ((teamID < 0) || (teamID >= MAX_TEAMS)) {
		return 0;
	}

	unitDrawer->DrawUnitDef(ud, teamID);

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
			logOutput.Print("LUA: error parsing numeric array\n");
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
	if ((args != 2) || !lua_isnumber(L, 1) || !lua_istable(L, 2)) {
		lua_pushstring(L, "Incorrect arguments to DrawShape(type, elements[])");
		lua_error(L);
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

static int DrawColor(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if (args < 1) {
		lua_pushstring(L, "Incorrect arguments to DrawColor()");
		lua_error(L);
	}

	float color[4];

	if (args == 1) {
		if (!lua_istable(L, 1)) {
			lua_pushstring(L, "Incorrect arguments to DrawColor()");
			lua_error(L);
		}
		const int count = ParseFloatArray(L, color, 4);
		if (count < 3) {
			lua_pushstring(L, "Incorrect arguments to DrawColor()");
			lua_error(L);
		}
		if (count == 3) {
			color[3] = 1.0f;
		}
	}
	else if (args >= 3) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
			lua_pushstring(L, "Incorrect arguments to DrawColor()");
			lua_error(L);
		}
		color[0] = (GLfloat)lua_tonumber(L, 1);
		color[1] = (GLfloat)lua_tonumber(L, 2);
		color[2] = (GLfloat)lua_tonumber(L, 3);
		if (args < 4) {
			color[3] = 1.0f;
		} else {
			if (!lua_isnumber(L, 4)) {
				lua_pushstring(L, "Incorrect arguments to DrawColor()");
				lua_error(L);
			}
			color[3] = (GLfloat)lua_tonumber(L, 4);
		}
	}
	else {
		lua_pushstring(L, "Incorrect arguments to DrawColor()");
		lua_error(L);
	}

	glColor4fv(color);

	return 0;
}


static int DrawMaterial(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_istable(L, 1)) {
		lua_pushstring(L,
			"Incorrect arguments to DrawMaterial(table)");
		lua_error(L);
	}

	float color[4];

	const int table = lua_gettop(L);
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_isstring(L, -2)) { // the key
			logOutput.Print("DrawMaterial: bad state type\n");
			break;
		}
		const string key = lua_tostring(L, -2);
		
		if (key == "shininess") {
			if (lua_isnumber(L, -1)) {
				const GLfloat specExp = (GLfloat)lua_tonumber(L, -1);
				glMaterialf(GL_FRONT_AND_BACK, GL_EMISSION, specExp);
			}
			continue;
		}

		const int count = ParseFloatArray(L, color, 4);
		if (count == 3) {
			color[3] = 1.0f;
		}

		if (key == "ambidiff") {
			if (count >= 3) {
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color);
			}
		}
		else if (key == "ambient") {
			if (count >= 3) {
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, color);
			}
		}
		else if (key == "diffuse") {
			if (count >= 3) {
				glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
			}
		}
		else if (key == "specular") {
			if (count >= 3) {
				glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
			}
		}
		else if (key == "emission") {
			if (count >= 3) {
				glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, color);
			}
		}
		else {
			logOutput.Print("DrawMaterial: unknown material type: %s\n", key.c_str());
		}
	}
	return 0;
}


/******************************************************************************/

static int DrawScreenGeometry(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
		lua_pushstring(L,
			"Incorrect arguments to DrawScreenProperties(width, dist)");
		lua_error(L);
	}
	screenWidth = lua_tonumber(L, 1);
	screenDistance = lua_tonumber(L, 2);

	return 0;
}


/******************************************************************************/

static void ResetGLState()
{
	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_LINE_STIPPLE);
	glAlphaFunc(GL_GREATER, 0.5f);
	glDisable(GL_LIGHTING);
	glDisable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_INVERT);
	glDisable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glShadeModel(GL_SMOOTH);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_SCISSOR_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_POLYGON_OFFSET_LINE);
	glDisable(GL_POLYGON_OFFSET_POINT);
	glLineWidth(1.0f);
	glPointSize(1.0f);
}


static int DrawResetState(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "DrawResetState takes no arguments");
		lua_error(L);
	}

	ResetGLState();

	return 0;
}


static int DrawLighting(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isboolean(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to DrawLighting()");
		lua_error(L);
	}
	if (lua_toboolean(L, 1)) {
		glEnable(GL_LIGHTING);
	} else {
		glDisable(GL_LIGHTING);
	}
	return 0;
}


static int DrawShadeModel(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to DrawShadeModel()");
		lua_error(L);
	}

	glShadeModel((GLenum)lua_tonumber(L, 1));

	return 0;
}


static int DrawScissor(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if (args == 1) {
		if (!lua_isboolean(L, 1)) {
			lua_pushstring(L, "Incorrect arguments to DrawScissor()");
			lua_error(L);
		}
		if (lua_toboolean(L, 1)) {
			glEnable(GL_SCISSOR_TEST);
		} else {
			glDisable(GL_SCISSOR_TEST);
		}
	}
	else if (args == 4) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) ||
		    !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
			lua_pushstring(L, "Incorrect arguments to DrawScissor()");
			lua_error(L);
		}
		glEnable(GL_SCISSOR_TEST);
		const GLint   x =   (GLint)lua_tonumber(L, 1);
		const GLint   y =   (GLint)lua_tonumber(L, 2);
		const GLsizei w = (GLsizei)lua_tonumber(L, 3);
		const GLsizei h = (GLsizei)lua_tonumber(L, 4);
		glScissor(x, y, w, h);
	}
	else {
		lua_pushstring(L, "Incorrect arguments to DrawScissor()");
		lua_error(L);
	}

	return 0;
}


static int DrawDepthMask(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isboolean(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to DrawDepthMask()");
		lua_error(L);
	}
	if (lua_toboolean(L, 1)) {
		glDepthMask(GL_TRUE);
	} else {
		glDepthMask(GL_FALSE);
	}
	return 0;
}


static int DrawDepthTest(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if (args != 1) {
		lua_pushstring(L, "Incorrect arguments to DrawDepthTest()");
		lua_error(L);
	}

	if (lua_isboolean(L, 1)) {
		if (lua_toboolean(L, 1)) {
			glEnable(GL_DEPTH_TEST);
		} else {
			glDisable(GL_DEPTH_TEST);
		}
	}
	else if (lua_isnumber(L, 1)) {
		glEnable(GL_DEPTH_TEST);
		glDepthFunc((GLenum)lua_tonumber(L, 1));
	}
	else {
		lua_pushstring(L, "Incorrect arguments to DrawDepthTest()");
		lua_error(L);
	}
	return 0;
}


static int DrawCulling(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if (args != 1) {
		lua_pushstring(L, "Incorrect arguments to DrawCulling()");
		lua_error(L);
	}

	if (lua_isboolean(L, 1)) {
		if (lua_toboolean(L, 1)) {
			glEnable(GL_CULL_FACE);
		} else {
			glDisable(GL_CULL_FACE);
		}
	}
	else if (lua_isnumber(L, 1)) {
		glEnable(GL_CULL_FACE);
		glCullFace((GLenum)lua_tonumber(L, 1));
	}
	else {
		lua_pushstring(L, "Incorrect arguments to DrawCulling()");
		lua_error(L);
	}
	return 0;
}


static int DrawLogicOp(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if (args != 1) {
		lua_pushstring(L, "Incorrect arguments to DrawLogicOp()");
		lua_error(L);
	}

	if (lua_isboolean(L, 1)) {
		if (lua_toboolean(L, 1)) {
			glEnable(GL_COLOR_LOGIC_OP);
		} else {
			glDisable(GL_COLOR_LOGIC_OP);
		}
	}
	else if (lua_isnumber(L, 1)) {
		glEnable(GL_COLOR_LOGIC_OP);
		glLogicOp((GLenum)lua_tonumber(L, 1));
	}
	else {
		lua_pushstring(L, "Incorrect arguments to DrawLogicOp()");
		lua_error(L);
	}
	return 0;
}


static int DrawBlending(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if (args == 1) {
		if (!lua_isboolean(L, 1)) {
			lua_pushstring(L, "Incorrect arguments to DrawBlending()");
			lua_error(L);
		}
		if (lua_toboolean(L, 1)) {
			glEnable(GL_BLEND);
		} else {
			glDisable(GL_BLEND);
		}
	}
	else if (args == 2) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
			lua_pushstring(L, "Incorrect arguments to DrawBlending()");
			lua_error(L);
		}
		glEnable(GL_BLEND);
		glBlendFunc((GLenum)lua_tonumber(L, 1), (GLenum)lua_tonumber(L, 2));
	}
	else {
		lua_pushstring(L, "Incorrect arguments to DrawBlending()");
		lua_error(L);
	}
	return 0;
}


static int DrawAlphaTest(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if (args == 1) {
		if (!lua_isboolean(L, 1)) {
			lua_pushstring(L, "Incorrect arguments to DrawAlphaTest()");
			lua_error(L);
		}
		if (lua_toboolean(L, 1)) {
			glEnable(GL_ALPHA_TEST);
		} else {
			glDisable(GL_ALPHA_TEST);
		}
	}
	else if (args == 2) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
			lua_pushstring(L, "Incorrect arguments to DrawAlphaTest()");
			lua_error(L);
		}
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc((GLenum)lua_tonumber(L, 1), (GLfloat)lua_tonumber(L, 2));
	}
	else {
		lua_pushstring(L, "Incorrect arguments to DrawAlphaTest()");
		lua_error(L);
	}
	return 0;
}


static int DrawPolygonMode(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
		lua_pushstring(L, "Incorrect arguments to DrawPolygonMode()");
		lua_error(L);
	}
	const GLenum face = (GLenum)lua_tonumber(L, 1);
	const GLenum mode = (GLenum)lua_tonumber(L, 2);
	glPolygonMode(face, mode);
	return 0;
}


static int DrawPolygonOffset(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if (args == 1) {
		if (!lua_isboolean(L, 1)) {
			lua_pushstring(L, "Incorrect arguments to DrawPolygonOffset()");
			lua_error(L);
		}
		if (lua_toboolean(L, 1)) {
			glEnable(GL_POLYGON_OFFSET_FILL);
			glEnable(GL_POLYGON_OFFSET_LINE);
			glEnable(GL_POLYGON_OFFSET_POINT);
		} else {
			glDisable(GL_POLYGON_OFFSET_FILL);
			glDisable(GL_POLYGON_OFFSET_LINE);
			glDisable(GL_POLYGON_OFFSET_POINT);
		}
	}
	else if (args == 2) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
			lua_pushstring(L, "Incorrect arguments to DrawPolygonOffset()");
			lua_error(L);
		}
		glEnable(GL_POLYGON_OFFSET_FILL);
		glEnable(GL_POLYGON_OFFSET_LINE);
		glEnable(GL_POLYGON_OFFSET_POINT);
		glPolygonOffset((GLfloat)lua_tonumber(L, 1), (GLfloat)lua_tonumber(L, 2));
	}
	else {
		lua_pushstring(L, "Incorrect arguments to DrawPolygonOffset()");
		lua_error(L);
	}
	return 0;
}


static int DrawLineStipple(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments

	if (args == 1) {
		if (lua_isstring(L, 1)) { // we're ignoring the string value
			const unsigned int stipPat = (0xffff & cmdColors.StipplePattern());
			if ((stipPat != 0x0000) && (stipPat != 0xffff)) {
				glEnable(GL_LINE_STIPPLE);
				lineDrawer.SetupLineStipple();
			} else {
				glDisable(GL_LINE_STIPPLE);
			}
		}
		else if (lua_isboolean(L, 1)) {
			if (lua_toboolean(L, 1)) {
				glEnable(GL_LINE_STIPPLE);
			} else {
				glDisable(GL_LINE_STIPPLE);
			}
		}
		else {
			lua_pushstring(L, "Incorrect arguments to DrawLineStipple()");
			lua_error(L);
		}
	}
	else if (args == 2) {
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
			lua_pushstring(L, "Incorrect arguments to DrawLineStipple()");
			lua_error(L);
		}
		GLint factor     =    (GLint)lua_tonumber(L, 1);
		GLushort pattern = (GLushort)lua_tonumber(L, 2);
		glEnable(GL_LINE_STIPPLE);
		glLineStipple(factor, pattern);
	}
	else {
		lua_pushstring(L, "Incorrect arguments to DrawLineStipple()");
		lua_error(L);
	}
	return 0;
}


static int DrawLineWidth(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to DrawLineWidth()");
		lua_error(L);
	}
	glLineWidth((float)lua_tonumber(L, 1));
	return 0;
}


static int DrawPointSize(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to DrawPointSize()");
		lua_error(L);
	}
	glPointSize((float)lua_tonumber(L, 1));
	return 0;
}


static int DrawTexture(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if (args != 1) {
		lua_pushstring(L, "Incorrect arguments to DrawTexture()");
		lua_error(L);
	}

	if (lua_isboolean(L, 1)) {
		if (lua_toboolean(L, 1)) {
			glEnable(GL_TEXTURE_2D);
		} else {
			glDisable(GL_TEXTURE_2D);
		}
		lua_pushboolean(L, 1);
		return 1;
	}
		
	if (!lua_isstring(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to DrawTexture()");
		lua_error(L);
	}

	const string texture = lua_tostring(L, 1);
	if (texture.empty()) {
		glDisable(GL_TEXTURE_2D); // equivalent to 'false'
		lua_pushboolean(L, 1);
		return 1;
	}
	else if (texture[0] != '#') {
		if ((guihandler == NULL) || !guihandler->BindNamedTexture(texture)) {
			lua_pushboolean(L, 0);
			return 1;
		}
		glEnable(GL_TEXTURE_2D);
	}
	else {
		// unit build picture
		char* endPtr;
		const char* startPtr = texture.c_str() + 1; // skip the '#'
		int unitDefID = (int)strtol(startPtr, &endPtr, 10);
		// UnitDefHandler's array size is (numUnits + 1)
		if (endPtr != startPtr) {
			UnitDef* ud = unitDefHandler->GetUnitByID(unitDefID);
			if (ud != NULL) {
				glBindTexture(GL_TEXTURE_2D, unitDefHandler->GetUnitImage(ud));
				glEnable(GL_TEXTURE_2D);
			} else {
				lua_pushboolean(L, 0);
				return 1;
			}
		} else {
			lua_pushboolean(L, 0);
			return 1;
		}
	}
	lua_pushboolean(L, 1);
	return 1;
}


static int DrawFreeTexture(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isstring(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to DrawFreeTexture()");
		lua_error(L);
	}
	const string texture = lua_tostring(L, 1);
	if ((guihandler == NULL) || !guihandler->FreeNamedTexture(texture)) {
		lua_pushboolean(L, 0);
	} else {
		lua_pushboolean(L, 1);
	}
	return 1;
}


static int DrawClear(lua_State* L)
{
	if (!drawingEnabled) {
		return 0;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args < 1) || !lua_isnumber(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to DrawClear()");
		lua_error(L);
	}

	const GLbitfield bits = (GLbitfield)lua_tonumber(L, 1);
	if (args == 5) {
		if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3) ||
		    !lua_isnumber(L, 4) || !lua_isnumber(L, 5)) {
			lua_pushstring(L, "Incorrect arguments to DrawClear()");
			lua_error(L);
		}
		if (bits == GL_COLOR_BUFFER_BIT) {
			glClearColor((GLfloat)lua_tonumber(L, 2), (GLfloat)lua_tonumber(L, 3),
			             (GLfloat)lua_tonumber(L, 4), (GLfloat)lua_tonumber(L, 5));
		}
		else if (bits == GL_ACCUM_BUFFER_BIT) {
			glClearAccum((GLfloat)lua_tonumber(L, 2), (GLfloat)lua_tonumber(L, 3),
			             (GLfloat)lua_tonumber(L, 4), (GLfloat)lua_tonumber(L, 5));
		}
	}
	else if (args == 2) {
		if (!lua_isnumber(L, 2)) {
			lua_pushstring(L, "Incorrect arguments to DrawClear()");
			lua_error(L);
		}
		if (bits == GL_DEPTH_BUFFER_BIT) {
			glClearDepth((GLfloat)lua_tonumber(L, 2));
		}
		else if (bits == GL_STENCIL_BUFFER_BIT) {
			glClearStencil((GLint)lua_tonumber(L, 2));
		}
	}

	glClear(bits);

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

static int DrawMatrixMode(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		lua_pushstring(L, "Incorrect arguments to DrawMatrixMode");
		lua_error(L);
	}
	glMatrixMode((GLenum)lua_tonumber(L, 1));
	return 0;
}


static int DrawLoadIdentity(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		lua_pushstring(L, "DrawLoadIdentity takes no arguments");
		lua_error(L);
	}
	glLoadIdentity();
	return 0;
}


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

	// generate the list id
	const GLuint list = glGenLists(1);
	if (list == 0) {
		lua_pushnumber(L, 0);
		return 1;
	}
		
	// save the current state
	const bool origDrawingEnabled = drawingEnabled;
	drawingEnabled = true;
	const int	origMatrixDepth = matrixDepth;
	matrixDepth = 0;

	// build the list with the specified lua call/args
	glNewList(list, GL_COMPILE);
	const int error = lua_pcall(L, (args - 1), 0, 0);
	glEndList();

	if (error != 0) {
		glDeleteLists(list, 1);
		lua_pushnumber(L, 0);
		logOutput.Print("DrawListCreate: error = %s\n", lua_tostring(L, -1));
	} else {
		displayLists.push_back(list);
		lua_pushnumber(L, displayLists.size());
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
	const unsigned int listIndex = (unsigned int)lua_tonumber(L, 1);
	if ((listIndex > 0) && (listIndex <= displayLists.size())) {
		glCallList(displayLists[listIndex - 1]);
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
	const unsigned int listIndex = (unsigned int)lua_tonumber(L, 1);
	if ((listIndex > 0) && (listIndex <= displayLists.size())) {
		glDeleteLists(displayLists[listIndex - 1], 1);
		displayLists[listIndex - 1] = 0;
	}
	return 0;
}


/******************************************************************************/
