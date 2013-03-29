/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaUnsyncedRead.h"

#include "LuaInclude.h"
#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaUtils.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/Game.h"
#include "Game/GameHelper.h"
#include "Game/GameServer.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Player.h"
#include "Game/PlayerHandler.h"
#include "Game/PlayerRoster.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/TraceRay.h"
#include "Game/Camera/CameraController.h"
#include "Game/UI/GuiHandler.h"
#include "Game/UI/InfoConsole.h"
#include "Game/UI/KeyCodes.h"
#include "Game/UI/KeySet.h"
#include "Game/UI/KeyBindings.h"
#include "Game/UI/MiniMap.h"
#include "Game/UI/MouseHandler.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/BaseGroundTextures.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/IconHandler.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Env/IWater.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "Sim/Units/Groups/Group.h"
#include "Sim/Units/Groups/GroupHandler.h"
#include "System/NetProtocol.h"
#include "System/Config/ConfigVariable.h"
#include "System/Input/KeyInput.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/LoadSave/demofile.h"
#include "System/LoadSave/DemoReader.h"
#include "System/Sound/SoundChannels.h"

#if !defined(HEADLESS) && !defined(NO_SOUND)
	#include "System/Sound/EFX.h"
	#include "System/Sound/EFXPresets.h"
#endif

#include <set>
#include <list>
#include <cctype>

#include <SDL_timer.h>
#include <SDL_keysym.h>
#include <SDL_mouse.h>

const int CMD_INDEX_OFFSET = 1; // starting index for command descriptions

/******************************************************************************/
/******************************************************************************/

bool LuaUnsyncedRead::PushEntries(lua_State* L)
{
#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(IsReplay);
	REGISTER_LUA_CFUNC(GetReplayLength);
	REGISTER_LUA_CFUNC(GetModUICtrl);

	REGISTER_LUA_CFUNC(GetDrawFrame);
	REGISTER_LUA_CFUNC(GetFrameTimeOffset);
	REGISTER_LUA_CFUNC(GetLastUpdateSeconds);
	REGISTER_LUA_CFUNC(GetHasLag);

	REGISTER_LUA_CFUNC(GetViewGeometry);
	REGISTER_LUA_CFUNC(GetWindowGeometry);
	REGISTER_LUA_CFUNC(GetScreenGeometry);
	REGISTER_LUA_CFUNC(GetMiniMapGeometry);
	REGISTER_LUA_CFUNC(GetMiniMapDualScreen);
	REGISTER_LUA_CFUNC(IsAboveMiniMap);

	REGISTER_LUA_CFUNC(IsAABBInView);
	REGISTER_LUA_CFUNC(IsSphereInView);

	REGISTER_LUA_CFUNC(IsUnitAllied);
	REGISTER_LUA_CFUNC(IsUnitInView);
	REGISTER_LUA_CFUNC(IsUnitVisible);
	REGISTER_LUA_CFUNC(IsUnitIcon);
	REGISTER_LUA_CFUNC(IsUnitSelected);

	REGISTER_LUA_CFUNC(GetUnitLuaDraw);
	REGISTER_LUA_CFUNC(GetUnitNoDraw);
	REGISTER_LUA_CFUNC(GetUnitNoMinimap);
	REGISTER_LUA_CFUNC(GetUnitNoSelect);
	REGISTER_LUA_CFUNC(GetFeatureLuaDraw);

	REGISTER_LUA_CFUNC(GetUnitTransformMatrix);
	REGISTER_LUA_CFUNC(GetUnitViewPosition);

	REGISTER_LUA_CFUNC(GetVisibleUnits);
	REGISTER_LUA_CFUNC(GetVisibleFeatures);

	REGISTER_LUA_CFUNC(GetTeamColor);
	REGISTER_LUA_CFUNC(GetTeamOrigColor);

	REGISTER_LUA_CFUNC(GetLocalPlayerID);
	REGISTER_LUA_CFUNC(GetLocalTeamID);
	REGISTER_LUA_CFUNC(GetLocalAllyTeamID);
	REGISTER_LUA_CFUNC(GetSpectatingState);

	REGISTER_LUA_CFUNC(GetSelectedUnits);
	REGISTER_LUA_CFUNC(GetSelectedUnitsSorted);
	REGISTER_LUA_CFUNC(GetSelectedUnitsCounts);
	REGISTER_LUA_CFUNC(GetSelectedUnitsCount);

	REGISTER_LUA_CFUNC(IsGUIHidden);
	REGISTER_LUA_CFUNC(HaveShadows);
	REGISTER_LUA_CFUNC(HaveAdvShading);
	REGISTER_LUA_CFUNC(GetWaterMode);
	REGISTER_LUA_CFUNC(GetMapDrawMode);
	REGISTER_LUA_CFUNC(GetMapSquareTexture);

	REGISTER_LUA_CFUNC(GetCameraNames);
	REGISTER_LUA_CFUNC(GetCameraState);
	REGISTER_LUA_CFUNC(GetCameraPosition);
	REGISTER_LUA_CFUNC(GetCameraDirection);
	REGISTER_LUA_CFUNC(GetCameraFOV);
	REGISTER_LUA_CFUNC(GetCameraVectors);
	REGISTER_LUA_CFUNC(WorldToScreenCoords);
	REGISTER_LUA_CFUNC(TraceScreenRay);

	REGISTER_LUA_CFUNC(GetTimer);
	REGISTER_LUA_CFUNC(DiffTimers);

	REGISTER_LUA_CFUNC(GetSoundStreamTime);
	REGISTER_LUA_CFUNC(GetSoundEffectParams);

	// moved from LuaUI

	REGISTER_LUA_CFUNC(GetFPS);

	REGISTER_LUA_CFUNC(GetActiveCommand);
	REGISTER_LUA_CFUNC(GetDefaultCommand);
	REGISTER_LUA_CFUNC(GetActiveCmdDescs);
	REGISTER_LUA_CFUNC(GetActiveCmdDesc);
	REGISTER_LUA_CFUNC(GetCmdDescIndex);

	REGISTER_LUA_CFUNC(GetBuildFacing);
	REGISTER_LUA_CFUNC(GetBuildSpacing);

	REGISTER_LUA_CFUNC(GetGatherMode);

	REGISTER_LUA_CFUNC(GetActivePage);

	REGISTER_LUA_CFUNC(GetMouseState);
	REGISTER_LUA_CFUNC(GetMouseCursor);
	REGISTER_LUA_CFUNC(GetMouseStartPosition);

	REGISTER_LUA_CFUNC(GetKeyState);
	REGISTER_LUA_CFUNC(GetModKeyState);
	REGISTER_LUA_CFUNC(GetPressedKeys);
	REGISTER_LUA_CFUNC(GetInvertQueueKey);

	REGISTER_LUA_CFUNC(GetKeyCode);
	REGISTER_LUA_CFUNC(GetKeySymbol);
	REGISTER_LUA_CFUNC(GetKeyBindings);
	REGISTER_LUA_CFUNC(GetActionHotKeys);

	REGISTER_LUA_CFUNC(GetLastMessagePositions);

	REGISTER_LUA_CFUNC(GetConsoleBuffer);
	REGISTER_LUA_CFUNC(GetCurrentTooltip);

	REGISTER_LUA_CFUNC(GetMyAllyTeamID);
	REGISTER_LUA_CFUNC(GetMyTeamID);
	REGISTER_LUA_CFUNC(GetMyPlayerID);

	REGISTER_LUA_CFUNC(GetGroupList);
	REGISTER_LUA_CFUNC(GetSelectedGroup);

	REGISTER_LUA_CFUNC(GetUnitGroup);

	REGISTER_LUA_CFUNC(GetGroupUnits);
	REGISTER_LUA_CFUNC(GetGroupUnitsSorted);
	REGISTER_LUA_CFUNC(GetGroupUnitsCounts);
	REGISTER_LUA_CFUNC(GetGroupUnitsCount);

	REGISTER_LUA_CFUNC(GetPlayerRoster);
	REGISTER_LUA_CFUNC(GetPlayerTraffic);
	REGISTER_LUA_CFUNC(GetPlayerStatistics);

	REGISTER_LUA_CFUNC(GetDrawSelectionInfo);

	REGISTER_LUA_CFUNC(GetConfigParams);

	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Parsing helpers
//

static inline CUnit* ParseUnit(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "%s(): Bad unitID", caller);
		return NULL;
	}
	const int unitID = lua_toint(L, index);
	if ((unitID < 0) || (static_cast<size_t>(unitID) >= unitHandler->MaxUnits())) {
		luaL_error(L, "%s(): Bad unitID: %i\n", caller, unitID);
	}
	CUnit* unit = unitHandler->units[unitID];
	if (unit == NULL) {
		return NULL;
	}
	const int readAllyTeam = CLuaHandle::GetHandleReadAllyTeam(L);
	if (readAllyTeam < 0) {
		return CLuaHandle::GetHandleFullRead(L) ? unit : NULL;
	}
	if ((unit->losStatus[readAllyTeam] & (LOS_INLOS | LOS_INRADAR)) == 0) {
		return NULL;
	}
	return unit;
}

static inline CFeature* ParseFeature(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "%s(): Bad featureID", caller);
		return NULL;
	}
	const int featureID = lua_toint(L, index);
	CFeature* feature = featureHandler->GetFeature(featureID);

	if (CLuaHandle::GetHandleFullRead(L)) { return feature; }

	const int readAllyTeam = CLuaHandle::GetHandleReadAllyTeam(L);
	if (readAllyTeam < 0) { return NULL; }
	if (feature == NULL) { return NULL; }
	if (feature->IsInLosForAllyTeam(readAllyTeam)) { return feature; }

	return NULL;
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
//  The call-outs
//

int LuaUnsyncedRead::IsReplay(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	if (gameSetup && gameSetup->hostDemo) {
		lua_pushboolean(L, true);
	} else {
		lua_pushboolean(L, false);
	}
	return 1;
}


int LuaUnsyncedRead::GetReplayLength(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	if (gameServer && gameServer->GetDemoReader()) {
		lua_pushnumber(L, gameServer->GetDemoReader()->GetFileHeader().gameTime);
		return 1;
	}
	return 0;
}


int LuaUnsyncedRead::GetModUICtrl(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushboolean(L, CLuaHandle::GetModUICtrl());
	return 1;
}


/******************************************************************************/

int LuaUnsyncedRead::GetViewGeometry(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, globalRendering->viewSizeX);
	lua_pushnumber(L, globalRendering->viewSizeY);
	lua_pushnumber(L, globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewPosY);
	return 4;
}


int LuaUnsyncedRead::GetWindowGeometry(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	const int winPosY_bl = globalRendering->screenSizeY - globalRendering->winSizeY - globalRendering->winPosY; //! origin BOTTOMLEFT
	lua_pushnumber(L, globalRendering->winSizeX);
	lua_pushnumber(L, globalRendering->winSizeY);
	lua_pushnumber(L, globalRendering->winPosX);
	lua_pushnumber(L, winPosY_bl);
	return 4;
}


int LuaUnsyncedRead::GetScreenGeometry(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, globalRendering->screenSizeX);
	lua_pushnumber(L, globalRendering->screenSizeY);
	lua_pushnumber(L, 0.0f);
	lua_pushnumber(L, 0.0f);
	return 4;
}


int LuaUnsyncedRead::GetMiniMapGeometry(lua_State* L)
{
	if (minimap == NULL) {
		return 0;
	}
	lua_pushnumber(L, minimap->GetPosX());
	lua_pushnumber(L, minimap->GetPosY());
	lua_pushnumber(L, minimap->GetSizeX());
	lua_pushnumber(L, minimap->GetSizeY());
	lua_pushboolean(L, minimap->GetMinimized());
	lua_pushboolean(L, minimap->GetMaximized());

	return 6;
}


int LuaUnsyncedRead::GetMiniMapDualScreen(lua_State* L)
{
	if (minimap == NULL) {
		return 0;
	}
	if (!globalRendering->dualScreenMode) {
		lua_pushboolean(L, false);
	} else {
		if (globalRendering->dualScreenMiniMapOnLeft) {
			lua_pushliteral(L, "left");
		} else {
			lua_pushliteral(L, "right");
		}
	}
	return 1;
}


int LuaUnsyncedRead::IsAboveMiniMap(lua_State* L)
{
	if (minimap == NULL) {
		return 0;
	}

	if (minimap->GetMinimized() || game->hideInterface) {
		return false;
	}

	const int x = luaL_checkint(L, 1) + globalRendering->viewPosX;
	const int y = luaL_checkint(L, 2);

	const int x0 = minimap->GetPosX();
	const int y0 = minimap->GetPosY();
	const int x1 = x0 + minimap->GetSizeX();
	const int y1 = y0 + minimap->GetSizeY();

	lua_pushboolean(L, (x >= x0) && (x < x1) &&
	                   (y >= y0) && (y < y1));

	return 1;
}


/******************************************************************************/

int LuaUnsyncedRead::GetDrawFrame(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, globalRendering->drawFrame & 0xFFFF);
	lua_pushnumber(L, (globalRendering->drawFrame >> 16) & 0xFFFF);
	return 2;
}


int LuaUnsyncedRead::GetFrameTimeOffset(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, globalRendering->timeOffset);
	return 1;
}


int LuaUnsyncedRead::GetLastUpdateSeconds(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, game->updateDeltaSeconds);
	return 1;
}

int LuaUnsyncedRead::GetHasLag(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushboolean(L, game ? game->HasLag() : false);
	return 1;
}

int LuaUnsyncedRead::IsAABBInView(lua_State* L)
{
	float3 mins = float3(luaL_checkfloat(L, 1),
	                     luaL_checkfloat(L, 2),
	                     luaL_checkfloat(L, 3));
	float3 maxs = float3(luaL_checkfloat(L, 4),
	                     luaL_checkfloat(L, 5),
	                     luaL_checkfloat(L, 6));
	lua_pushboolean(L, camera->InView(mins, maxs));
	return 1;
}


int LuaUnsyncedRead::IsSphereInView(lua_State* L)
{
	const float3 pos(luaL_checkfloat(L, 1),
	                 luaL_checkfloat(L, 2),
	                 luaL_checkfloat(L, 3));
	const float radius = lua_israwnumber(L, 4) ? lua_tofloat(L, 4) : 0.0f;

	lua_pushboolean(L, camera->InView(pos, radius));
	return 1;
}


int LuaUnsyncedRead::IsUnitAllied(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		lua_pushboolean(L, CLuaHandle::GetHandleFullRead(L));
	} else {
		lua_pushboolean(L, (unit->allyteam == CLuaHandle::GetHandleReadAllyTeam(L)));
	}
	return 1;
}


int LuaUnsyncedRead::IsUnitInView(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushboolean(L, camera->InView(unit->midPos, unit->radius));
	return 1;
}


static bool UnitIsIcon(const CUnit* unit)
{
	const float sqDist = (unit->pos - camera->pos).SqLength();
	const float iconLength = unitDrawer->iconLength;
	const float iconDistSqrMult = unit->unitDef->iconType->GetDistanceSqr();
	const float realIconLength = iconLength * iconDistSqrMult;
	return (sqDist > realIconLength);
}


int LuaUnsyncedRead::IsUnitVisible(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const float radius = luaL_optnumber(L, 2, unit->radius);
	const bool checkIcon = lua_toboolean(L, 3);

	const int readAllyTeam = CLuaHandle::GetHandleReadAllyTeam(L);

	if (readAllyTeam < 0) {
		if (!CLuaHandle::GetHandleFullRead(L)) {
			lua_pushboolean(L, false);
		} else {
			lua_pushboolean(L,
				(!checkIcon || !UnitIsIcon(unit)) &&
				camera->InView(unit->midPos, radius));
		}
	}
	else {
		if ((unit->losStatus[readAllyTeam] & LOS_INLOS) == 0) {
			lua_pushboolean(L, false);
		} else {
			lua_pushboolean(L,
				(!checkIcon || !UnitIsIcon(unit)) &&
				camera->InView(unit->midPos, radius));
		}
	}
	return 1;
}


int LuaUnsyncedRead::IsUnitIcon(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushboolean(L, UnitIsIcon(unit));
	return 1;
}


int LuaUnsyncedRead::IsUnitSelected(lua_State* L)
{
	GML_RECMUTEX_LOCK(sel); // IsUnitSelected

	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const CUnitSet& selUnits = selectedUnitsHandler.selectedUnits;
	lua_pushboolean(L, selUnits.find(unit) != selUnits.end());
	return 1;
}


int LuaUnsyncedRead::GetUnitLuaDraw(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushboolean(L, unit->luaDraw);
	return 1;
}

int LuaUnsyncedRead::GetUnitNoDraw(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushboolean(L, unit->noDraw);
	return 1;
}

int LuaUnsyncedRead::GetUnitNoMinimap(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushboolean(L, unit->noMinimap);
	return 1;
}

int LuaUnsyncedRead::GetUnitNoSelect(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	lua_pushboolean(L, unit->noSelect);
	return 1;
}


int LuaUnsyncedRead::GetFeatureLuaDraw(lua_State* L)
{
	CFeature* feature = ParseFeature(L, __FUNCTION__, 1);
	if (feature == NULL) {
		return 0;
	}
	lua_pushboolean(L, feature->luaDraw);
	return 1;
}



int LuaUnsyncedRead::GetUnitTransformMatrix(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}

	CMatrix44f m = unit->GetTransformMatrix(false, false);

	if ((lua_isboolean(L, 2) && lua_toboolean(L, 2))) {
		m = m.InvertAffine();
	}

	for (int i = 0; i < 16; i += 4) {
		lua_pushnumber(L, m[i    ]);
		lua_pushnumber(L, m[i + 1]);
		lua_pushnumber(L, m[i + 2]);
		lua_pushnumber(L, m[i + 3]);
	}

	return 16;
}

int LuaUnsyncedRead::GetUnitViewPosition(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const bool midPos = (lua_isboolean(L, 2) && lua_toboolean(L, 2));

	float3& pos = midPos ? unit->drawMidPos : unit->drawPos;
	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, pos.z);
	return 3;
}


/******************************************************************************/
/******************************************************************************/

// FIXME -- copied from LuaSyncedRead.cpp, commonize
enum UnitAllegiance {
	AllUnits   = -1,
	MyUnits    = -2,
	AllyUnits  = -3,
	EnemyUnits = -4
};


class CUnitQuads : public CReadMap::IQuadDrawer
{
public:
	CUnitQuads() : count(0) {};

	int count;
	std::vector<const std::list<CUnit*>*> visunits;

	void DrawQuad(int x, int y)
	{
		const CQuadField::Quad& q = quadField->GetQuadAt(x,y);
		if (!q.units.empty())
			visunits.push_back(&q.units);
		count += q.units.size();
	}
};


class CFeatureQuads : public CReadMap::IQuadDrawer
{
public:
	CFeatureQuads() : count(0) {};

	int count;
	std::vector<const std::list<CFeature*>*> visfeatures;

	void DrawQuad(int x, int y)
	{
		const CQuadField::Quad& q = quadField->GetQuadAt(x,y);
		if (!q.features.empty())
			visfeatures.push_back(&q.features);
		count += q.features.size();
	}
};


int LuaUnsyncedRead::GetVisibleUnits(lua_State* L)
{
	// arg 1 - teamID
	int teamID = luaL_optint(L, 1, -1);
	if (teamID == MyUnits) {
		const int scriptTeamID = CLuaHandle::GetHandleReadTeam(L);
		if (scriptTeamID >= 0) {
			teamID = scriptTeamID;
		} else {
			teamID = AllUnits;
		}
	}
	int allyTeamID = CLuaHandle::GetHandleReadAllyTeam(L);
	if (teamID >= 0) {
		allyTeamID = teamHandler->AllyTeam(teamID);
	}
	if (allyTeamID < 0) {
		if (!CLuaHandle::GetHandleFullRead(L)) {
			return 0;
		}
	}

	// arg 2 - unit radius
	bool fixedRadius = false;
	float radius = 30.0f; // value from UnitDrawer.cpp
	if (lua_israwnumber(L, 2)) {
		radius = lua_tofloat(L, 2);
		if (radius < 0.0f) {
			fixedRadius = true;
			radius = -radius;
		}
	}

	// arg 3 - noIcons
	const bool noIcons = lua_isboolean(L, 3) && !lua_toboolean(L, 3);

	vector<const CUnitSet*> unitSets;
	static CUnitSet visQuadUnits;

	CUnitQuads quadIter;
	int count = 0;

	{
		GML_RECMUTEX_LOCK(quad); // GetVisibleUnits

		readmap->GridVisibility(camera, CQuadField::QUAD_SIZE / SQUARE_SIZE, 1e9, &quadIter, INT_MAX);

		lua_createtable(L, quadIter.count, 0);

		// setup the list of unit sets
		if (quadIter.count > unitHandler->activeUnits.size()/3) {
			// if we see nearly all features, it is just faster to check them all, instead of doing slow duplication checks
			if (teamID >= 0) {
				unitSets.push_back(&teamHandler->Team(teamID)->units);
			} else {
				for (int t = 0; t < teamHandler->ActiveTeams(); t++) {
					if ((teamID == AllUnits) ||
						((teamID == AllyUnits)  && (allyTeamID == teamHandler->AllyTeam(t))) ||
						((teamID == EnemyUnits) && (allyTeamID != teamHandler->AllyTeam(t))))
					{
						unitSets.push_back(&teamHandler->Team(t)->units);
					}
				}
			}
		} else {
			// objects can exist in multiple quads, so we still need to do a duplication check
			visQuadUnits.clear();
			std::vector<const std::list<CUnit*>*>::iterator sit;
			for (sit = quadIter.visunits.begin(); sit != quadIter.visunits.end(); ++sit) {
				std::list<CUnit*>::const_iterator unitIt;
				for (unitIt = (*sit)->begin(); unitIt != (*sit)->end(); ++unitIt) {
					CUnit* unit = *unitIt;
					if ((teamID == AllUnits) ||
						((teamID >= 0) && (teamID == unit->team)) ||
						((teamID == AllyUnits)  && (allyTeamID == unit->allyteam)) ||
						((teamID == EnemyUnits) && (allyTeamID != unit->allyteam)))
					{
						visQuadUnits.insert(unit);
					}
				}
			}
			unitSets.push_back(&visQuadUnits);
		}
	}

	const float iconLength = unitDrawer->iconLength;

	vector<const CUnitSet*>::const_iterator setIt;
	for (setIt = unitSets.begin(); setIt != unitSets.end(); ++setIt) {
		const CUnitSet* unitSet = *setIt;

		CUnitSet::const_iterator unitIt;
		for (unitIt = unitSet->begin(); unitIt != unitSet->end(); ++unitIt) {
			const CUnit& unit = **unitIt;

			if (unit.noDraw) {
				continue;
			}

			if (allyTeamID >= 0) {
				if (!(unit.losStatus[allyTeamID] & LOS_INLOS)) {
					continue;
				}
			}

			if (noIcons) {
				const float sqDist = (unit.pos - camera->pos).SqLength();
				const float iconDistSqrMult = unit.unitDef->iconType->GetDistanceSqr();
				const float realIconLength = iconLength * iconDistSqrMult;
				if (sqDist > realIconLength) {
					continue;
				}
			}

			const float testRadius = fixedRadius ? radius : (unit.drawRadius + radius);
			if (!camera->InView(unit.midPos, testRadius)) {
				continue;
			}

			//! add the unit
			count++;
			lua_pushnumber(L, count);
			lua_pushnumber(L, unit.id);
			lua_rawset(L, -3);
		}
	}

	return 1;
}


int LuaUnsyncedRead::GetVisibleFeatures(lua_State* L)
{
	// arg 1 - allyTeamID
	int allyTeamID = luaL_optint(L, 1, -1);
	if (allyTeamID >= 0 && !teamHandler->ValidAllyTeam(allyTeamID)) {
		return 0;
	}
	if (allyTeamID < 0) {
		allyTeamID = -1;
		if (!CLuaHandle::GetHandleFullRead(L)) {
			allyTeamID = CLuaHandle::GetHandleReadAllyTeam(L);
		}
	}

	// arg 2 - feature radius
	bool fixedRadius = false;
	float radius = 30.0f; // value from UnitDrawer.cpp
	if (lua_israwnumber(L, 2)) {
		radius = lua_tofloat(L, 2);
		if (radius < 0.0f) {
			fixedRadius = true;
			radius = -radius;
		}
	}

	const bool noIcons = lua_isboolean(L, 3) && !lua_toboolean(L, 3);
	const bool noGeos = lua_isboolean(L, 4) && !lua_toboolean(L, 4);

	bool scanAll = false;
	static CFeatureSet visQuadFeatures;
	static CFeatureQuads quadIter;
	int count = 0;

	{
		GML_RECMUTEX_LOCK(quad); // GetVisibleFeatures

		readmap->GridVisibility(camera, CQuadField::QUAD_SIZE / SQUARE_SIZE, 3000.0f * 2.0f, &quadIter, INT_MAX);

		lua_createtable(L, quadIter.count, 0);

		//! setup the list of features
		if (quadIter.count > featureHandler->GetActiveFeatures().size()/3) {
			//! if we see nearly all features, it is just faster to check them all, instead of doing slow duplication checks
			scanAll = true;
		} else {
			//! features can exist in multiple quads, so we need to do a duplication check
			visQuadFeatures.clear();
			std::vector<const std::list<CFeature*>*>::iterator it;
			for (it = quadIter.visfeatures.begin(); it != quadIter.visfeatures.end(); ++it) {
				std::list<CFeature*>::const_iterator featureIt;
				for (featureIt = (*it)->begin(); featureIt != (*it)->end(); ++featureIt) {
					visQuadFeatures.insert(*featureIt);
				}
			}
		}
	}

	const CFeatureSet& featureSet = (scanAll) ? featureHandler->GetActiveFeatures() : visQuadFeatures;

	for (CFeatureSet::const_iterator featureIt = featureSet.begin(); featureIt != featureSet.end(); ++featureIt) {
		const CFeature& f = **featureIt;

		if (noGeos && f.def->geoThermal)
			continue;

		if (noIcons) {
			float sqDist = (f.pos - camera->pos).SqLength2D();
			float farLength = f.sqRadius * unitDrawer->unitDrawDist * unitDrawer->unitDrawDist;
			if (sqDist >= farLength) {
				continue;
			}
		}

		if (!gu->spectatingFullView && !f.IsInLosForAllyTeam(allyTeamID)) {
			continue;
		}

		const float testRadius = fixedRadius ? radius : (f.drawRadius + radius);
		if (!camera->InView(f.midPos, testRadius)) {
			continue;
		}

		// add the unit
		count++;
		lua_pushnumber(L, count);
		lua_pushnumber(L, f.id);
		lua_rawset(L, -3);
	}

	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetLocalPlayerID(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, gu->myPlayerNum);
	return 1;
}


int LuaUnsyncedRead::GetLocalTeamID(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, gu->myTeam);
	return 1;
}


int LuaUnsyncedRead::GetLocalAllyTeamID(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, gu->myAllyTeam);
	return 1;
}


int LuaUnsyncedRead::GetSpectatingState(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushboolean(L, gu->spectating);
	lua_pushboolean(L, gu->spectatingFullView);
	lua_pushboolean(L, gu->spectatingFullSelect);
	return 3;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetSelectedUnits(lua_State* L)
{
	GML_RECMUTEX_LOCK(sel); // GetSelectedUnits

	CheckNoArgs(L, __FUNCTION__);
	lua_newtable(L);
	int count = 0;
	const CUnitSet& selUnits = selectedUnitsHandler.selectedUnits;
	CUnitSet::const_iterator it;
	for (it = selUnits.begin(); it != selUnits.end(); ++it) {
		count++;
		lua_pushnumber(L, count);
		lua_pushnumber(L, (*it)->id);
		lua_rawset(L, -3);
	}
	return 1;
}


int LuaUnsyncedRead::GetSelectedUnitsSorted(lua_State* L)
{
	GML_RECMUTEX_LOCK(sel); // GetSelectedUnitsSorted

	CheckNoArgs(L, __FUNCTION__);

	map<int, vector<CUnit*> > unitDefMap;
	const CUnitSet& selUnits = selectedUnitsHandler.selectedUnits;
	CUnitSet::const_iterator it;
	for (it = selUnits.begin(); it != selUnits.end(); ++it) {
		CUnit* unit = *it;
		unitDefMap[unit->unitDef->id].push_back(unit);
	}

	lua_newtable(L);
	map<int, vector<CUnit*> >::const_iterator mit;
	for (mit = unitDefMap.begin(); mit != unitDefMap.end(); ++mit) {
		lua_pushnumber(L, mit->first); // push the UnitDef index
		lua_newtable(L); {
			const vector<CUnit*>& v = mit->second;
			for (int i = 0; i < (int)v.size(); i++) {
				CUnit* unit = v[i];
				lua_pushnumber(L, i + 1);
				lua_pushnumber(L, unit->id);
				lua_rawset(L, -3);
			}
		}
		lua_rawset(L, -3);
	}

	// UnitDef ID keys are not necessarily consecutive
	HSTR_PUSH_NUMBER(L, "n", unitDefMap.size());
	return 1;
}


int LuaUnsyncedRead::GetSelectedUnitsCounts(lua_State* L)
{
	GML_RECMUTEX_LOCK(sel); // GetSelectedUnitsCounts

	CheckNoArgs(L, __FUNCTION__);

	// tally the types
	map<int, int> countMap;
	const CUnitSet& selUnits = selectedUnitsHandler.selectedUnits;
	CUnitSet::const_iterator it;
	for (it = selUnits.begin(); it != selUnits.end(); ++it) {
		CUnit* unit = *it;
		map<int, int>::iterator mit = countMap.find(unit->unitDef->id);
		if (mit == countMap.end()) {
			countMap[unit->unitDef->id] = 1;
		} else {
			mit->second++;
		}
	}

	lua_newtable(L);
	map<int, int>::const_iterator mit;
	for (mit = countMap.begin(); mit != countMap.end(); ++mit) {
		lua_pushnumber(L, mit->first);  // push the UnitDef index
		lua_pushnumber(L, mit->second); // push the UnitDef unit count
		lua_rawset(L, -3);
	}

	// UnitDef ID keys are not necessarily consecutive
	HSTR_PUSH_NUMBER(L, "n", countMap.size());
	return 1;
}


int LuaUnsyncedRead::GetSelectedUnitsCount(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, selectedUnitsHandler.selectedUnits.size());
	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::IsGUIHidden(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	if (game == NULL) {
		return 0;
	}
	lua_pushboolean(L, game->hideInterface);
	return 1;
}


int LuaUnsyncedRead::HaveShadows(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	if (shadowHandler == NULL) {
		return 0;
	}
	lua_pushboolean(L, shadowHandler->shadowsLoaded);
	return 1;
}


int LuaUnsyncedRead::HaveAdvShading(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	if (unitDrawer == NULL) {
		return 0;
	}
	lua_pushboolean(L, unitDrawer->advShading);
	return 1;
}


int LuaUnsyncedRead::GetWaterMode(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	if (water == NULL) {
		return 0;
	}

	const int mode = water->GetID();
	const char* modeName = water->GetName();

	lua_pushnumber(L, mode);
	lua_pushstring(L, modeName);
	return 2;
}


int LuaUnsyncedRead::GetMapDrawMode(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	const CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
	switch (gd->drawMode) {
		case CBaseGroundDrawer::drawNormal:             { HSTR_PUSH(L, "normal"            ); break; }
		case CBaseGroundDrawer::drawHeight:             { HSTR_PUSH(L, "height"            ); break; }
		case CBaseGroundDrawer::drawMetal:              { HSTR_PUSH(L, "metal"             ); break; }
		case CBaseGroundDrawer::drawLos:                { HSTR_PUSH(L, "los"               ); break; }
		case CBaseGroundDrawer::drawPathTraversability: { HSTR_PUSH(L, "pathTraversability"); break; }
		case CBaseGroundDrawer::drawPathHeat:           { HSTR_PUSH(L, "pathHeat"          ); break; }
		case CBaseGroundDrawer::drawPathFlow:           { HSTR_PUSH(L, "pathFlow"          ); break; }
		case CBaseGroundDrawer::drawPathCost:           { HSTR_PUSH(L, "pathCost"          ); break; }
	}
	return 1;
}


int LuaUnsyncedRead::GetMapSquareTexture(lua_State* L)
{
	if (CLuaHandle::GetHandleSynced(L)) {
		return 0;
	}

	const int texSquareX = luaL_checkint(L, 1);
	const int texSquareY = luaL_checkint(L, 2);
	const int texMipLevel = luaL_checkint(L, 3);
	const std::string& texName = luaL_checkstring(L, 4);

	CBaseGroundDrawer* groundDrawer = readmap->GetGroundDrawer();
	CBaseGroundTextures* groundTextures = groundDrawer->GetGroundTextures();

	if (groundTextures == NULL) {
		lua_pushboolean(L, false);
		return 1;
	}
	if (texName.empty()) {
		lua_pushboolean(L, false);
		return 1;
	}

	const LuaTextures& luaTextures = CLuaHandle::GetActiveTextures(L);
	const LuaTextures::Texture* luaTexture = luaTextures.GetInfo(texName);

	if (luaTexture == NULL) {
		// not a valid texture (name)
		lua_pushboolean(L, false);
		return 1;
	}

	const int tid = luaTexture->id;
	const int txs = luaTexture->xsize;
	const int tys = luaTexture->ysize;

	if (txs != tys) {
		// square textures only
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushboolean(L, groundTextures->GetSquareLuaTexture(texSquareX, texSquareY, tid, txs, tys, texMipLevel));
	return 1;
}

/******************************************************************************/

int LuaUnsyncedRead::GetCameraNames(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);

	lua_newtable(L);
	const std::vector<CCameraController*>& cc = camHandler->GetAvailableControllers();
	for (size_t i = 0; i < cc.size(); ++i) {
		lua_pushsstring(L, cc[i]->GetName());
		lua_pushnumber(L, i);
		lua_rawset(L, -3);
	}

	return 1;
}


int LuaUnsyncedRead::GetCameraState(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);

	lua_newtable(L);

	lua_pushliteral(L, "name");
	lua_pushsstring(L, camHandler->GetCurrentControllerName());
	lua_rawset(L, -3);

	CCameraController::StateMap camState;
	CCameraController::StateMap::const_iterator it;
	camHandler->GetState(camState);
	for (it = camState.begin(); it != camState.end(); ++it) {
		lua_pushsstring(L, it->first);
		lua_pushnumber(L, it->second);
		lua_rawset(L, -3);
	}

	return 1;
}


int LuaUnsyncedRead::GetCameraPosition(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, camera->pos.x);
	lua_pushnumber(L, camera->pos.y);
	lua_pushnumber(L, camera->pos.z);
	return 3;
}


int LuaUnsyncedRead::GetCameraDirection(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, camera->forward.x);
	lua_pushnumber(L, camera->forward.y);
	lua_pushnumber(L, camera->forward.z);
	return 3;
}


int LuaUnsyncedRead::GetCameraFOV(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, camera->GetFov());
	return 1;
}


int LuaUnsyncedRead::GetCameraVectors(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);

#define PACK_CAMERA_VECTOR(n) \
	HSTR_PUSH(L, #n);           \
	lua_createtable(L, 3, 0);            \
	lua_pushnumber(L, camera-> n .x); lua_rawseti(L, -2, 1); \
	lua_pushnumber(L, camera-> n .y); lua_rawseti(L, -2, 2); \
	lua_pushnumber(L, camera-> n .z); lua_rawseti(L, -2, 3); \
	lua_rawset(L, -3)

	lua_newtable(L);
	PACK_CAMERA_VECTOR(forward);
	PACK_CAMERA_VECTOR(up);
	PACK_CAMERA_VECTOR(right);
	PACK_CAMERA_VECTOR(topFrustumSideDir);
	PACK_CAMERA_VECTOR(botFrustumSideDir);
	PACK_CAMERA_VECTOR(lftFrustumSideDir);
	PACK_CAMERA_VECTOR(rgtFrustumSideDir);

	return 1;
}


int LuaUnsyncedRead::WorldToScreenCoords(lua_State* L)
{
	const float3 worldPos(luaL_checkfloat(L, 1),
	                      luaL_checkfloat(L, 2),
	                      luaL_checkfloat(L, 3));
	const float3 winPos = camera->CalcWindowCoordinates(worldPos);
	lua_pushnumber(L, winPos.x);
	lua_pushnumber(L, winPos.y);
	lua_pushnumber(L, winPos.z);
	return 3;
}


int LuaUnsyncedRead::TraceScreenRay(lua_State* L)
{
	// window coordinates
	const int mx = luaL_checkint(L, 1);
	const int my = luaL_checkint(L, 2);
	const bool onlyCoords  = (lua_isboolean(L, 3) && lua_toboolean(L, 3));
	const bool useMiniMap  = (lua_isboolean(L, 4) && lua_toboolean(L, 4));
	const bool includeSky  = (lua_isboolean(L, 5) && lua_toboolean(L, 5));
	const bool ignoreWater = (lua_isboolean(L, 6) && lua_toboolean(L, 6));

	const int wx = mx + globalRendering->viewPosX;
	const int wy = globalRendering->viewSizeY - 1 - my - globalRendering->viewPosY;

	if (useMiniMap && (minimap != NULL) && !minimap->GetMinimized()) {
		const int px = minimap->GetPosX() - globalRendering->viewPosX; // for left dualscreen
		const int py = minimap->GetPosY();
		const int sx = minimap->GetSizeX();
		const int sy = minimap->GetSizeY();
		if ((mx >= px) && (mx < (px + sx)) &&
		    (my >= py) && (my < (py + sy))) {
			const float3 pos = minimap->GetMapPosition(wx, wy);
			if (!onlyCoords) {
				const CUnit* unit = minimap->GetSelectUnit(pos);
				if (unit != NULL) {
					lua_pushliteral(L, "unit");
					lua_pushnumber(L, unit->id);
					return 2;
				}
			}
			const float posY = ground->GetHeightReal(pos.x, pos.z, false);
			lua_pushliteral(L, "ground");
			lua_createtable(L, 3, 0);
			lua_pushnumber(L, pos.x); lua_rawseti(L, -2, 1);
			lua_pushnumber(L, posY);  lua_rawseti(L, -2, 2);
			lua_pushnumber(L, pos.z); lua_rawseti(L, -2, 3);
			return 2;
		}
	}

	if ((mx < 0) || (mx >= globalRendering->viewSizeX) ||
	    (my < 0) || (my >= globalRendering->viewSizeY)) {
		return 0;
	}

	CUnit* unit = NULL;
	CFeature* feature = NULL;

	const float range = globalRendering->viewRange * 1.4f;
	const float badRange = range - 300.0f;

	const float3& pos = camera->pos;
	const float3 dir = camera->CalcPixelDir(wx, wy);


// FIXME	const int origAllyTeam = gu->myAllyTeam;
//	gu->myAllyTeam = readAllyTeam;
	const float dist = TraceRay::GuiTraceRay(pos, dir, range, NULL, unit, feature, true, onlyCoords, ignoreWater);
//	gu->myAllyTeam = origAllyTeam;

	if ((dist < 0.0f || dist > badRange) && unit == NULL && feature == NULL) {
		// ray went into the void (or started too far above terrain)
		if (includeSky) {
			lua_pushliteral(L, "sky");
		} else {
			return 0;
		}
	} else {
		if (!onlyCoords) {
			if (unit != NULL) {
				lua_pushliteral(L, "unit");
				lua_pushnumber(L, unit->id);
				return 2;
			}

			if (feature != NULL) {
				lua_pushliteral(L, "feature");
				lua_pushnumber(L, feature->id);
				return 2;
			}
		}

		lua_pushliteral(L, "ground");
	}

	const float3 groundPos = pos + (dir * dist);
	lua_createtable(L, 3, 0);
	lua_pushnumber(L, groundPos.x); lua_rawseti(L, -2, 1);
	lua_pushnumber(L, groundPos.y); lua_rawseti(L, -2, 2);
	lua_pushnumber(L, groundPos.z); lua_rawseti(L, -2, 3);

	return 2;
}


/******************************************************************************/

static void AddPlayerToRoster(lua_State* L, int playerID, bool includePathingFlag)
{
#define PUSH_ROSTER_ENTRY(type, val) \
	lua_pushnumber(L, index); index++; \
	lua_push ## type(L, val); lua_rawset(L, -3);

	const CPlayer* p = playerHandler->Player(playerID);
	int index = 1;
	lua_newtable(L);
	PUSH_ROSTER_ENTRY(string, p->name.c_str());
	PUSH_ROSTER_ENTRY(number, playerID);
	PUSH_ROSTER_ENTRY(number, p->team);
	PUSH_ROSTER_ENTRY(number, teamHandler->AllyTeam(p->team));
	PUSH_ROSTER_ENTRY(boolean, p->spectator);
	PUSH_ROSTER_ENTRY(number, p->cpuUsage);
	const float pingScale = (GAME_SPEED * gs->speedFactor);

	if (!includePathingFlag || p->ping != PATHING_FLAG) {
		const float pingSecs = float(p->ping - 1) / pingScale;
		PUSH_ROSTER_ENTRY(number, pingSecs);
	} else {
		const float pingSecs = float(p->ping);
		PUSH_ROSTER_ENTRY(number, pingSecs);
	}
}


int LuaUnsyncedRead::GetTeamColor(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);
	if ((teamID < 0) || (teamID >= teamHandler->ActiveTeams())) {
		return 0;
	}
	const CTeam* team = teamHandler->Team(teamID);
	if (team == NULL) {
		return 0;
	}

	lua_pushnumber(L, (float)team->color[0] / 255.0f);
	lua_pushnumber(L, (float)team->color[1] / 255.0f);
	lua_pushnumber(L, (float)team->color[2] / 255.0f);
	lua_pushnumber(L, (float)team->color[3] / 255.0f);

	return 4;
}


int LuaUnsyncedRead::GetTeamOrigColor(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);
	if ((teamID < 0) || (teamID >= teamHandler->ActiveTeams())) {
		return 0;
	}
	const CTeam* team = teamHandler->Team(teamID);
	if (team == NULL) {
		return 0;
	}

	lua_pushnumber(L, (float)team->origColor[0] / 255.0f);
	lua_pushnumber(L, (float)team->origColor[1] / 255.0f);
	lua_pushnumber(L, (float)team->origColor[2] / 255.0f);
	lua_pushnumber(L, (float)team->origColor[3] / 255.0f);

	return 4;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetTimer(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushlightuserdata(L, (void*)SDL_GetTicks());
	return 1;
}


int LuaUnsyncedRead::DiffTimers(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 2) || !lua_isuserdata(L, 1) || !lua_isuserdata(L, 2)) {
		luaL_error(L, "Incorrect arguments to DiffTimers()");
	}
	const void* p1 = lua_touserdata(L, 1);
	const void* p2 = lua_touserdata(L, 2);
	const Uint32 t1 = *((const Uint32*)&p1);
	const Uint32 t2 = *((const Uint32*)&p2);
	const Uint32 diffTime = (t1 - t2);
	lua_pushnumber(L, (float)diffTime * 0.001f); // return seconds
	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetSoundStreamTime(lua_State* L)
{
	lua_pushnumber(L, Channels::BGMusic.StreamGetPlayTime());
	lua_pushnumber(L, Channels::BGMusic.StreamGetTime());
	return 2;
}


int LuaUnsyncedRead::GetSoundEffectParams(lua_State* L)
{
#if defined(HEADLESS) || defined(NO_SOUND)
	return 0;
#else
	if (!efx || !efx->sfxProperties)
		return 0;

	EAXSfxProps* efxprops = efx->sfxProperties;

	lua_createtable(L, 0, 2);

	size_t n = efxprops->filter_properties_f.size();
	lua_pushliteral(L, "passfilter");
	lua_createtable(L, 0, n);
	lua_rawset(L, -3);
	for (std::map<ALuint, ALfloat>::iterator it = efxprops->filter_properties_f.begin(); it != efxprops->filter_properties_f.end(); ++it)
	{
		const ALuint param = it->first;
		std::map<ALuint, std::string>::iterator fit = alFilterParamToName.find(param);
		if (fit != alFilterParamToName.end()) {
			const std::string& name = fit->second;
			lua_pushsstring(L, name);
			lua_pushnumber(L, it->second);
			lua_rawset(L, -3);
		}
	}


	n = efxprops->properties_v.size() + efxprops->properties_f.size() + efxprops->properties_i.size();
	lua_pushliteral(L, "reverb");
	lua_createtable(L, 0, n);
	lua_rawset(L, -3);
	for (std::map<ALuint, ALfloat>::iterator it = efxprops->properties_f.begin(); it != efxprops->properties_f.end(); ++it)
	{
		const ALuint param = it->first;
		std::map<ALuint, std::string>::iterator fit = alParamToName.find(param);
		if (fit != alParamToName.end()) {
			const std::string& name = fit->second;
			lua_pushsstring(L, name);
			lua_pushnumber(L, it->second);
			lua_rawset(L, -3);
		}
	}
	for (std::map<ALuint, float3>::iterator it = efxprops->properties_v.begin(); it != efxprops->properties_v.end(); ++it)
	{
		const ALuint param = it->first;
		std::map<ALuint, std::string>::iterator fit = alParamToName.find(param);
		if (fit != alParamToName.end()) {
			const float3& v = it->second;
			const std::string& name = fit->second;
			lua_pushsstring(L, name);
			lua_createtable(L, 3, 0);
				lua_pushnumber(L, v.x);
				lua_rawseti(L, -2, 1);
				lua_pushnumber(L, v.y);
				lua_rawseti(L, -2, 2);
				lua_pushnumber(L, v.z);
				lua_rawseti(L, -2, 3);
			lua_rawset(L, -3);
		}
	}
	for (std::map<ALuint, ALint>::iterator it = efxprops->properties_i.begin(); it != efxprops->properties_i.end(); ++it)
	{
		const ALuint param = it->first;
		std::map<ALuint, std::string>::iterator fit = alParamToName.find(param);
		if (fit != alParamToName.end()) {
			const std::string& name = fit->second;
			lua_pushsstring(L, name);
			lua_pushboolean(L, it->second);
			lua_rawset(L, -3);
		}
	}

	return 1;
#endif //! defined(HEADLESS) || defined(NO_SOUND)
}


/******************************************************************************/
/******************************************************************************/
//
// moved from LuaUI
//
/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetFPS(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	if (globalRendering) {
		lua_pushnumber(L, (int)globalRendering->FPS);
	} else {
		lua_pushnumber(L, 0);
	}
	return 1;
}


/******************************************************************************/

int LuaUnsyncedRead::GetActiveCommand(lua_State* L)
{
	GML_RECMUTEX_LOCK(gui); // GetActiveCommand

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
	lua_pushsstring(L, cmdDescs[inCommand].name);
	return 4;
}


int LuaUnsyncedRead::GetDefaultCommand(lua_State* L)
{
	if (guihandler == NULL) {
		return 0;
	}
	CheckNoArgs(L, __FUNCTION__);

	const int defCmd = guihandler->GetDefaultCommand(mouse->lastx, mouse->lasty);

	GML_RECMUTEX_LOCK(gui); // GetDefaultCommand

	const vector<CommandDescription>& cmdDescs = guihandler->commands;
	const int cmdDescCount = (int)cmdDescs.size();

	lua_pushnumber(L, defCmd + CMD_INDEX_OFFSET);
	if ((defCmd < 0) || (defCmd >= cmdDescCount)) {
		return 1;
	}
	lua_pushnumber(L, cmdDescs[defCmd].id);
	lua_pushnumber(L, cmdDescs[defCmd].type);
	lua_pushsstring(L, cmdDescs[defCmd].name);
	return 4;
}


int LuaUnsyncedRead::GetActiveCmdDescs(lua_State* L)
{
	GML_RECMUTEX_LOCK(gui); // GetActiveCmdDescs

	if (guihandler == NULL) {
		return 0;
	}
	CheckNoArgs(L, __FUNCTION__);

	const vector<CommandDescription>& cmdDescs = guihandler->commands;
	const int cmdDescCount = (int)cmdDescs.size();

	lua_checkstack(L, 1 + 2);
	lua_newtable(L);

	for (int i = 0; i < cmdDescCount; i++) {
		lua_pushnumber(L, i + CMD_INDEX_OFFSET);
		LuaUtils::PushCommandDesc(L, cmdDescs[i]);
		lua_rawset(L, -3);
	}
	return 1;
}


int LuaUnsyncedRead::GetActiveCmdDesc(lua_State* L)
{
	GML_RECMUTEX_LOCK(gui); // GetActiveCmdDesc

	if (guihandler == NULL) {
		return 0;
	}
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetActiveCmdDesc()");
	}
	const int cmdIndex = lua_toint(L, 1) - CMD_INDEX_OFFSET;

	const vector<CommandDescription>& cmdDescs = guihandler->commands;
	const int cmdDescCount = (int)cmdDescs.size();
	if ((cmdIndex < 0) || (cmdIndex >= cmdDescCount)) {
		return 0;
	}
	LuaUtils::PushCommandDesc(L, cmdDescs[cmdIndex]);
	return 1;
}


int LuaUnsyncedRead::GetCmdDescIndex(lua_State* L)
{
	GML_RECMUTEX_LOCK(gui); // GetCmdDescIndex

	if (guihandler == NULL) {
		return 0;
	}
	const int cmdId = luaL_checkint(L, 1);

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

int LuaUnsyncedRead::GetBuildFacing(lua_State* L)
{
	if (guihandler == NULL) {
		return 0;
	}
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, guihandler->buildFacing);
	return 1;
}


int LuaUnsyncedRead::GetBuildSpacing(lua_State* L)
{
	if (guihandler == NULL) {
		return 0;
	}
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, guihandler->buildSpacing);
	return 1;
}


int LuaUnsyncedRead::GetGatherMode(lua_State* L)
{
	if (guihandler == NULL) {
		return 0;
	}
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, guihandler->GetGatherMode());
	return 1;
}


/******************************************************************************/

int LuaUnsyncedRead::GetActivePage(lua_State* L)
{
	if (guihandler == NULL) {
		return 0;
	}
	lua_pushnumber(L, guihandler->GetActivePage());
	lua_pushnumber(L, guihandler->GetMaxPage());
	return 2;
}


/******************************************************************************/

int LuaUnsyncedRead::GetMouseState(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, mouse->lastx - globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewSizeY - mouse->lasty - 1);
	lua_pushboolean(L, mouse->buttons[SDL_BUTTON_LEFT].pressed);
	lua_pushboolean(L, mouse->buttons[SDL_BUTTON_MIDDLE].pressed);
	lua_pushboolean(L, mouse->buttons[SDL_BUTTON_RIGHT].pressed);
	return 5;
}


int LuaUnsyncedRead::GetMouseCursor(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushsstring(L, mouse->GetCurrentCursor());
	lua_pushnumber(L, mouse->GetCurrentCursorScale());
	return 2;
}


int LuaUnsyncedRead::GetMouseStartPosition(lua_State* L)
{
	if (mouse == NULL) {
		return 0;
	}
	const int button = luaL_checkint(L, 1);
	if ((button <= 0) || (button > NUM_BUTTONS)) {
		return 0;
	}
	const CMouseHandler::ButtonPressEvt& bp = mouse->buttons[button];
	lua_pushnumber(L, bp.x);
	lua_pushnumber(L, bp.y);
	lua_pushnumber(L, bp.camPos.x);
	lua_pushnumber(L, bp.camPos.y);
	lua_pushnumber(L, bp.camPos.z);
	lua_pushnumber(L, bp.dir.x);
	lua_pushnumber(L, bp.dir.y);
	lua_pushnumber(L, bp.dir.z);
	return 8;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetKeyState(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetKeyState(keycode)");
	}
	const int key = lua_toint(L, 1);
	if ((key < 0) || (key >= SDLK_LAST)) {
		lua_pushboolean(L, 0);
	} else {
		lua_pushboolean(L, keyInput->GetKeyState(key));
	}
	return 1;
}


int LuaUnsyncedRead::GetModKeyState(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushboolean(L, keyInput->GetKeyState(SDLK_LALT));
	lua_pushboolean(L, keyInput->GetKeyState(SDLK_LCTRL));
	lua_pushboolean(L, keyInput->GetKeyState(SDLK_LMETA));
	lua_pushboolean(L, keyInput->GetKeyState(SDLK_LSHIFT));
	return 4;
}


int LuaUnsyncedRead::GetPressedKeys(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_newtable(L);
	int count = 0;
	for (int i = 0; i < SDLK_LAST; i++) {
		if (keyInput->GetKeyState(i)) {
			lua_pushboolean(L, 1);
			lua_rawseti(L, -2, i);
			count++;
		}
	}
	return 1;
}


int LuaUnsyncedRead::GetInvertQueueKey(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	if (guihandler == NULL) {
		return 0;
	}
	lua_pushboolean(L, guihandler->GetInvertQueueKey());
	return 1;
}

/******************************************************************************/

int LuaUnsyncedRead::GetLastMessagePositions(lua_State* L)
{
	CInfoConsole* ic = game->infoConsole;
	if (ic == NULL) {
		return 0;
	}

	CheckNoArgs(L, __FUNCTION__);
	lua_newtable(L);
	for (int i=1; i<=ic->GetMsgPosCount(); i++) {
		lua_pushnumber(L, i);
		lua_newtable(L); {
			const float3 msgpos = ic->GetMsgPos();
			lua_pushnumber(L, 1);
			lua_pushnumber(L, msgpos.x);
			lua_rawset(L, -3);
			lua_pushnumber(L, 2);
			lua_pushnumber(L, msgpos.y);
			lua_rawset(L, -3);
			lua_pushnumber(L, 3);
			lua_pushnumber(L, msgpos.z);
			lua_rawset(L, -3);
		}
		lua_rawset(L, -3);
	}
	return 1;
}

/******************************************************************************/

int LuaUnsyncedRead::GetConsoleBuffer(lua_State* L)
{
	CInfoConsole* ic = game->infoConsole;
	if (ic == NULL) {
		return true;
	}

	const int args = lua_gettop(L); // number of arguments
	if ((args != 0) && ((args != 1) || !lua_isnumber(L, 1))) {
		luaL_error(L, "Incorrect arguments to GetConsoleBuffer([count])");
	}

	std::deque<CInfoConsole::RawLine> lines;
	ic->GetRawLines(lines);
	const int lineCount = (int)lines.size();

	int start = 0;
	if (args >= 1) {
		const int maxLines = lua_toint(L, 1);
		if (maxLines < lineCount) {
			start = (lineCount - maxLines);
		}
	}

	// table = { [1] = { text = string, zone = number}, etc... }
	lua_newtable(L);
	int count = 0;
	for (int i = start; i < lineCount; i++) {
		count++;
		lua_pushnumber(L, count);
		lua_newtable(L); {
			lua_pushliteral(L, "text");
			lua_pushsstring(L, lines[i].text);
			lua_rawset(L, -3);
			// FIXME migrate priority to level...
			lua_pushliteral(L, "priority");
			lua_pushnumber(L, 0);
			//lua_pushstring(L, lines[i].level);
			lua_rawset(L, -3);
		}
		lua_rawset(L, -3);
	}

	return 1;
}


int LuaUnsyncedRead::GetCurrentTooltip(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	const string tooltip = mouse->GetCurrentTooltip();
	lua_pushsstring(L, tooltip);
	return 1;
}


int LuaUnsyncedRead::GetKeyCode(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isstring(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetKeyCode(\"keysym\")");
	}
	const string keysym = lua_tostring(L, 1);
	lua_pushnumber(L, keyCodes->GetCode(keysym));
	return 1;
}


int LuaUnsyncedRead::GetKeySymbol(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args != 1) || !lua_isnumber(L, 1)) {
		luaL_error(L, "Incorrect arguments to GetKeySymbol(keycode)");
	}
	const int keycode = lua_toint(L, 1);
	lua_pushsstring(L, keyCodes->GetName(keycode));
	lua_pushsstring(L, keyCodes->GetDefaultName(keycode));
	return 2;
}


int LuaUnsyncedRead::GetKeyBindings(lua_State* L)
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
		const Action& action = actions[i];
		lua_pushnumber(L, i + 1);
		lua_newtable(L);
		lua_pushsstring(L, action.command);
		lua_pushsstring(L, action.extra);
		lua_rawset(L, -3);
		lua_rawset(L, -3);
	}
	return 1;
}


int LuaUnsyncedRead::GetActionHotKeys(lua_State* L)
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
		lua_pushsstring(L, hotkey);
		lua_rawset(L, -3);
	}
	return 1;
}

/******************************************************************************/

int LuaUnsyncedRead::GetGroupList(lua_State* L)
{
	GML_RECMUTEX_LOCK(group); // GetGroupList

	CheckNoArgs(L, __FUNCTION__);
	if (grouphandlers[gu->myTeam] == NULL) {
		return 0;
	}
	lua_newtable(L);
	int count = 0;
	const vector<CGroup*>& groups = grouphandlers[gu->myTeam]->groups;
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


int LuaUnsyncedRead::GetSelectedGroup(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, selectedUnitsHandler.GetSelectedGroup());
	return 1;
}


int LuaUnsyncedRead::GetUnitGroup(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	if ((unit->team == gu->myTeam) && (unit->group)) {
		lua_pushnumber(L, unit->group->id);
		return 1;
	}
	return 0;
}


/******************************************************************************/

int LuaUnsyncedRead::GetGroupUnits(lua_State* L)
{
	GML_RECMUTEX_LOCK(group); // GetGroupUnits

	const int groupID = luaL_checkint(L, 1);
	const vector<CGroup*>& groups = grouphandlers[gu->myTeam]->groups;
	if ((groupID < 0) || ((size_t)groupID >= groups.size()) ||
	    (groups[groupID] == NULL)) {
		return 0; // nils
	}

	lua_newtable(L);
	int count = 0;
	const CUnitSet& groupUnits = groups[groupID]->units;
	CUnitSet::const_iterator it;
	for (it = groupUnits.begin(); it != groupUnits.end(); ++it) {
		count++;
		lua_pushnumber(L, count);
		lua_pushnumber(L, (*it)->id);
		lua_rawset(L, -3);
	}

	return 1;
}


int LuaUnsyncedRead::GetGroupUnitsSorted(lua_State* L)
{
	GML_RECMUTEX_LOCK(group); // GetGroupUnitsSorted

	const int groupID = luaL_checkint(L, 1);
	const vector<CGroup*>& groups = grouphandlers[gu->myTeam]->groups;
	if ((groupID < 0) || ((size_t)groupID >= groups.size()) ||
	    (groups[groupID] == NULL)) {
		return 0; // nils
	}

	map<int, vector<CUnit*> > unitDefMap;
	const CUnitSet& groupUnits = groups[groupID]->units;
	CUnitSet::const_iterator it;
	for (it = groupUnits.begin(); it != groupUnits.end(); ++it) {
		CUnit* unit = *it;
		unitDefMap[unit->unitDef->id].push_back(unit);
	}

	lua_newtable(L);
	map<int, vector<CUnit*> >::const_iterator mit;
	for (mit = unitDefMap.begin(); mit != unitDefMap.end(); ++mit) {
		lua_pushnumber(L, mit->first); // push the UnitDef index
		lua_newtable(L); {
			const vector<CUnit*>& v = mit->second;
			for (int i = 0; i < (int)v.size(); i++) {
				CUnit* unit = v[i];
				lua_pushnumber(L, i + 1);
				lua_pushnumber(L, unit->id);
				lua_rawset(L, -3);
			}
		}
		lua_rawset(L, -3);
	}

	return 1;
}


int LuaUnsyncedRead::GetGroupUnitsCounts(lua_State* L)
{
	GML_RECMUTEX_LOCK(group); // GetGroupUnitsCounts

	const int groupID = luaL_checkint(L, 1);
	const vector<CGroup*>& groups = grouphandlers[gu->myTeam]->groups;
	if ((groupID < 0) || ((size_t)groupID >= groups.size()) ||
	    (groups[groupID] == NULL)) {
		return 0; // nils
	}

	map<int, int> countMap;
	const CUnitSet& groupUnits = groups[groupID]->units;
	CUnitSet::const_iterator it;
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
	for (mit = countMap.begin(); mit != countMap.end(); ++mit) {
		lua_pushnumber(L, mit->first);  // push the UnitDef index
		lua_pushnumber(L, mit->second); // push the UnitDef unit count
		lua_rawset(L, -3);
	}

	return 1;
}


int LuaUnsyncedRead::GetGroupUnitsCount(lua_State* L)
{
	GML_RECMUTEX_LOCK(group); // GetGroupUnitsCount

	const int groupID = luaL_checkint(L, 1);
	const vector<CGroup*>& groups = grouphandlers[gu->myTeam]->groups;
	if ((groupID < 0) || ((size_t)groupID >= groups.size()) ||
	    (groups[groupID] == NULL)) {
		return 0; // nils
	}
	lua_pushnumber(L, groups[groupID]->units.size());
	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetMyPlayerID(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, gu->myPlayerNum);
	return 1;
}


int LuaUnsyncedRead::GetMyTeamID(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, gu->myTeam);
	return 1;
}


int LuaUnsyncedRead::GetMyAllyTeamID(lua_State* L)
{
	CheckNoArgs(L, __FUNCTION__);
	lua_pushnumber(L, gu->myAllyTeam);
	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetPlayerRoster(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if ((args > 2) || ((args == 1 && !lua_isnumber(L, 1))) || (args == 2 && !lua_isboolean(L, 2))) {
		luaL_error(L, "Incorrect arguments to %s([ rosterSortType [, includePathingFlag] ])", __FUNCTION__);
	}

	const PlayerRoster::SortType oldSortType = playerRoster.GetSortType();

	if (args >= 1) {
		playerRoster.SetSortTypeByCode((PlayerRoster::SortType) lua_toint(L, 1));
	}

	const bool includePathingFlag = (args == 2 && lua_toboolean(L, 2));

	int count;
	const std::vector<int>& players = playerRoster.GetIndices(&count, includePathingFlag);

	playerRoster.SetSortTypeByCode(oldSortType); // revert

	lua_createtable(L, count, 0);
	for (int i = 0; i < count; i++) {
		AddPlayerToRoster(L, players[i], includePathingFlag);
		lua_rawseti(L, -2, i + 1);
	}

	return 1;
}

int LuaUnsyncedRead::GetPlayerTraffic(lua_State* L)
{
	const int playerID = luaL_checkint(L, 1);
	const int packetID = (int)luaL_optnumber(L, 2, -1);

	const std::map<int, CGame::PlayerTrafficInfo>& traffic
		= game->GetPlayerTraffic();
	std::map<int, CGame::PlayerTrafficInfo>::const_iterator it;
	it = traffic.find(playerID);
	if (it == traffic.end()) {
		lua_pushnumber(L, -1);
		return 1;
	}

	// only allow viewing stats for specific packet types
	if ((playerID != -1) &&              // all system counts can be read
	    (playerID != gu->myPlayerNum) && // all  self  counts can be read
	    (packetID != -1) &&
	    (packetID != NETMSG_CHAT)     &&
	    (packetID != NETMSG_PAUSE)    &&
	    (packetID != NETMSG_LUAMSG)   &&
	    (packetID != NETMSG_STARTPOS) &&
	    (packetID != NETMSG_USER_SPEED)) {
    lua_pushnumber(L, -1);
    return 1;
  }

	const CGame::PlayerTrafficInfo& pti = it->second;
	if (packetID == -1) {
		lua_pushnumber(L, pti.total);
		return 1;
	}
	std::map<int, int>::const_iterator pit = pti.packets.find(packetID);
	if (pit == pti.packets.end()) {
		lua_pushnumber(L, -1);
		return 1;
	}
	lua_pushnumber(L, pit->second);
	return 1;
}

int LuaUnsyncedRead::GetPlayerStatistics(lua_State* L)
{
	const int playerID = luaL_checkint(L, 1);
	if (!playerHandler->IsValidPlayer(playerID)) {
		return 0;
	}

	const CPlayer* player = playerHandler->Player(playerID);
	if (player == NULL) {
		return 0;
	}

	const PlayerStatistics& pStats = player->currentStats;

	lua_pushnumber(L, pStats.mousePixels);
	lua_pushnumber(L, pStats.mouseClicks);
	lua_pushnumber(L, pStats.keyPresses);
	lua_pushnumber(L, pStats.numCommands);
	lua_pushnumber(L, pStats.unitCommands);

	return 5;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetDrawSelectionInfo(lua_State* L)
{
	const int args = lua_gettop(L); // number of arguments
	if (args != 0) {
		luaL_error(L, "Incorrect arguments to GetDrawSelectionInfo()");
	}

	lua_pushboolean(L, guihandler ? guihandler->GetDrawSelectionInfo() : 0);
	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetConfigParams(lua_State* L)
{
	ConfigVariable::MetaDataMap cfgmap = ConfigVariable::GetMetaDataMap();
	lua_createtable(L, cfgmap.size(), 0);

	int i = 1;
	for (ConfigVariable::MetaDataMap::const_iterator it = cfgmap.begin(); it != cfgmap.end(); ++it)
	{
		const ConfigVariableMetaData* meta = it->second;

		lua_createtable(L, 0, 9);

			lua_pushsstring(L, "name");
			lua_pushsstring(L, meta->GetKey());
			lua_rawset(L, -3);
			if (meta->GetDescription().IsSet()) {
				lua_pushsstring(L, "description");
				lua_pushsstring(L, meta->GetDescription().ToString());
				lua_rawset(L, -3);
			}
			lua_pushsstring(L, "type");
			lua_pushsstring(L, meta->GetType());
			lua_rawset(L, -3);
			if (meta->GetDefaultValue().IsSet()) {
				lua_pushsstring(L, "defaultValue");
				lua_pushsstring(L, meta->GetDefaultValue().ToString());
				lua_rawset(L, -3);
			}
			if (meta->GetMinimumValue().IsSet()) {
				lua_pushsstring(L, "minimumValue");
				lua_pushsstring(L, meta->GetMinimumValue().ToString());
				lua_rawset(L, -3);
			}
			if (meta->GetMaximumValue().IsSet()) {
				lua_pushsstring(L, "maximumValue");
				lua_pushsstring(L, meta->GetMaximumValue().ToString());
				lua_rawset(L, -3);
			}
			if (meta->GetSafemodeValue().IsSet()) {
				lua_pushsstring(L, "safemodeValue");
				lua_pushsstring(L, meta->GetSafemodeValue().ToString());
				lua_rawset(L, -3);
			}
			if (meta->GetDeclarationFile().IsSet()) {
				lua_pushsstring(L, "declarationFile");
				lua_pushsstring(L, meta->GetDeclarationFile().ToString());
				lua_rawset(L, -3);
			}
			if (meta->GetDeclarationLine().IsSet()) {
				lua_pushsstring(L, "declarationLine");
				lua_pushnumber(L, meta->GetDeclarationLine().Get());
				lua_rawset(L, -3);
			}
			if (meta->GetReadOnly().IsSet()) {
				lua_pushsstring(L, "readOnly");
				lua_pushboolean(L, !!meta->GetReadOnly().Get());
				lua_rawset(L, -3);
			}

		lua_rawseti(L, -2, i++);
	}
	return 1;
}

/******************************************************************************/
/******************************************************************************/
