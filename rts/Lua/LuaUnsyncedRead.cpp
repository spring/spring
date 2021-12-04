/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaUnsyncedRead.h"

#include "LuaConfig.h"
#include "LuaInclude.h"
#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaUtils.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/Game.h"
#include "Game/GameHelper.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Game/IVideoCapturing.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
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
#include "Game/UI/PlayerRoster.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/BaseGroundTextures.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Menu/LuaMenuController.h"
#include "Net/GameServer.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GlobalRenderingInfo.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/Env/Decals/DecalsDrawerGL4.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/CommandAI/CommandDescription.h"
#include "Game/UI/Groups/Group.h"
#include "Game/UI/Groups/GroupHandler.h"
#include "Net/Protocol/NetProtocol.h" // NETMSG_*
#include "System/TimeProfiler.h"
#include "System/Config/ConfigHandler.h"
#include "System/Config/ConfigVariable.h"
#include "System/Input/KeyInput.h"
#include "System/LoadSave/DemoReader.h"
#include "System/Log/DefaultFilter.h"
#include "System/Platform/SDL1_keysym.h"
#include "System/Sound/ISoundChannels.h"
#include "System/Misc/SpringTime.h"

#if !defined(HEADLESS) && !defined(NO_SOUND)
	#include "System/Sound/OpenAL/EFX.h"
	#include "System/Sound/OpenAL/EFXPresets.h"
#endif

#include <cctype>

#include <SDL_clipboard.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>


/******************************************************************************/
/******************************************************************************/

bool LuaUnsyncedRead::PushEntries(lua_State* L)
{
	REGISTER_LUA_CFUNC(IsReplay);
	REGISTER_LUA_CFUNC(GetReplayLength);

	REGISTER_LUA_CFUNC(GetGameName);
	REGISTER_LUA_CFUNC(GetMenuName);

	REGISTER_LUA_CFUNC(GetProfilerTimeRecord);
	REGISTER_LUA_CFUNC(GetProfilerRecordNames);

	REGISTER_LUA_CFUNC(GetLuaMemUsage);
	REGISTER_LUA_CFUNC(GetVidMemUsage);

	REGISTER_LUA_CFUNC(GetDrawFrame);
	REGISTER_LUA_CFUNC(GetFrameTimeOffset);
	REGISTER_LUA_CFUNC(GetLastUpdateSeconds);
	REGISTER_LUA_CFUNC(GetVideoCapturingMode);

	REGISTER_LUA_CFUNC(GetViewGeometry);
	REGISTER_LUA_CFUNC(GetWindowGeometry);
	REGISTER_LUA_CFUNC(GetScreenGeometry);
	REGISTER_LUA_CFUNC(GetMiniMapGeometry);
	REGISTER_LUA_CFUNC(GetMiniMapDualScreen);
	REGISTER_LUA_CFUNC(IsAboveMiniMap);
	REGISTER_LUA_CFUNC(IsGUIHidden);

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
	REGISTER_LUA_CFUNC(GetUnitSelectionVolumeData);
	REGISTER_LUA_CFUNC(GetFeatureLuaDraw);
	REGISTER_LUA_CFUNC(GetFeatureNoDraw);
	REGISTER_LUA_CFUNC(GetFeatureSelectionVolumeData);

	REGISTER_LUA_CFUNC(GetUnitPieceTransformMatrices);
	REGISTER_LUA_CFUNC(GetFeaturePieceTransformMatrices);

	REGISTER_LUA_CFUNC(GetUnitTransformMatrix);
	REGISTER_LUA_CFUNC(GetFeatureTransformMatrix);

	REGISTER_LUA_CFUNC(GetUnitViewPosition);

	REGISTER_LUA_CFUNC(GetVisibleUnits);
	REGISTER_LUA_CFUNC(GetVisibleFeatures);
	REGISTER_LUA_CFUNC(GetVisibleProjectiles);

	REGISTER_LUA_CFUNC(GetTeamColor);
	REGISTER_LUA_CFUNC(GetTeamOrigColor);

	REGISTER_LUA_CFUNC(GetLocalPlayerID);
	REGISTER_LUA_CFUNC(GetLocalTeamID);
	REGISTER_LUA_CFUNC(GetLocalAllyTeamID);

	// aliases
	REGISTER_NAMED_LUA_CFUNC("GetMyPlayerID", GetLocalPlayerID);
	REGISTER_NAMED_LUA_CFUNC("GetMyTeamID", GetLocalTeamID);
	REGISTER_NAMED_LUA_CFUNC("GetMyAllyTeamID", GetLocalAllyTeamID);

	REGISTER_LUA_CFUNC(GetSpectatingState);

	REGISTER_LUA_CFUNC(GetSelectedUnits);
	REGISTER_LUA_CFUNC(GetSelectedUnitsSorted);
	REGISTER_LUA_CFUNC(GetSelectedUnitsCounts);
	REGISTER_LUA_CFUNC(GetSelectedUnitsCount);

	REGISTER_LUA_CFUNC(HaveShadows);
	REGISTER_LUA_CFUNC(HaveAdvShading);
	REGISTER_LUA_CFUNC(GetWaterMode);
	REGISTER_LUA_CFUNC(GetMapDrawMode);
	REGISTER_LUA_CFUNC(GetMapSquareTexture);

	REGISTER_LUA_CFUNC(GetLosViewColors);

	REGISTER_LUA_CFUNC(GetCameraNames);
	REGISTER_LUA_CFUNC(GetCameraState);
	REGISTER_LUA_CFUNC(GetCameraPosition);
	REGISTER_LUA_CFUNC(GetCameraDirection);
	REGISTER_LUA_CFUNC(GetCameraFOV);
	REGISTER_LUA_CFUNC(GetCameraVectors);
	REGISTER_LUA_CFUNC(WorldToScreenCoords);
	REGISTER_LUA_CFUNC(TraceScreenRay);
	REGISTER_LUA_CFUNC(GetPixelDir);

	REGISTER_LUA_CFUNC(GetTimer);
	REGISTER_LUA_CFUNC(GetFrameTimer);
	REGISTER_LUA_CFUNC(DiffTimers);

	REGISTER_LUA_CFUNC(GetSoundStreamTime);
	REGISTER_LUA_CFUNC(GetSoundEffectParams);

	REGISTER_LUA_CFUNC(GetFPS);
	REGISTER_LUA_CFUNC(GetGameSpeed);
	REGISTER_LUA_CFUNC(GetGameState);

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

	REGISTER_LUA_CFUNC(GetClipboard);

	REGISTER_LUA_CFUNC(GetKeyCode);
	REGISTER_LUA_CFUNC(GetKeySymbol);
	REGISTER_LUA_CFUNC(GetKeyBindings);
	REGISTER_LUA_CFUNC(GetActionHotKeys);

	REGISTER_LUA_CFUNC(GetLastMessagePositions);
	REGISTER_LUA_CFUNC(GetConsoleBuffer);
	REGISTER_LUA_CFUNC(GetCurrentTooltip);
	REGISTER_LUA_CFUNC(IsUserWriting);

	REGISTER_LUA_CFUNC(GetUnitGroup);
	REGISTER_LUA_CFUNC(GetGroupList);
	REGISTER_LUA_CFUNC(GetSelectedGroup);
	REGISTER_LUA_CFUNC(GetGroupUnits);
	REGISTER_LUA_CFUNC(GetGroupUnitsSorted);
	REGISTER_LUA_CFUNC(GetGroupUnitsCounts);
	REGISTER_LUA_CFUNC(GetGroupUnitsCount);

	REGISTER_LUA_CFUNC(GetPlayerRoster);
	REGISTER_LUA_CFUNC(GetPlayerTraffic);
	REGISTER_LUA_CFUNC(GetPlayerStatistics);

	REGISTER_LUA_CFUNC(GetDrawSelectionInfo);

	REGISTER_LUA_CFUNC(GetConfigParams);
	REGISTER_LUA_CFUNC(GetConfigInt);
	REGISTER_LUA_CFUNC(GetConfigFloat);
	REGISTER_LUA_CFUNC(GetConfigString);
	REGISTER_LUA_CFUNC(GetLogSections);

	REGISTER_LUA_CFUNC(GetAllDecals);
	REGISTER_LUA_CFUNC(GetDecalPos);
	REGISTER_LUA_CFUNC(GetDecalSize);
	REGISTER_LUA_CFUNC(GetDecalRotation);
	REGISTER_LUA_CFUNC(GetDecalTexture);
	REGISTER_LUA_CFUNC(GetDecalAlpha);
	REGISTER_LUA_CFUNC(GetDecalOwner);
	REGISTER_LUA_CFUNC(GetDecalType);

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
		luaL_error(L, "%s(): unitID not a number", caller);
		return nullptr;
	}

	CUnit* unit = unitHandler.GetUnit(lua_toint(L, index));

	if (unit == nullptr)
		return nullptr;

	const int readAllyTeam = CLuaHandle::GetHandleReadAllyTeam(L);

	if (readAllyTeam < 0)
		return CLuaHandle::GetHandleFullRead(L) ? unit : nullptr;

	if ((unit->losStatus[readAllyTeam] & (LOS_INLOS | LOS_INRADAR)) == 0)
		return nullptr;

	return unit;
}

static inline CFeature* ParseFeature(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "%s(): Bad featureID", caller);
		return nullptr;
	}

	CFeature* feature = featureHandler.GetFeature(lua_toint(L, index));

	if (CLuaHandle::GetHandleFullRead(L))
		return feature;

	const int readAllyTeam = CLuaHandle::GetHandleReadAllyTeam(L);

	if (readAllyTeam < 0)
		return nullptr;
	if (feature == nullptr)
		return nullptr;
	if (feature->IsInLosForAllyTeam(readAllyTeam))
		return feature;

	return nullptr;
}




static int GetSolidObjectLuaDraw(lua_State* L, const CSolidObject* obj)
{
	if (obj == nullptr)
		return 0;

	lua_pushboolean(L, obj->luaDraw);
	return 1;
}

static int GetSolidObjectNoDraw(lua_State* L, const CSolidObject* obj)
{
	if (obj == nullptr)
		return 0;

	lua_pushboolean(L, obj->noDraw);
	return 1;
}

static int GetSolidObjectSelectionVolume(lua_State* L, const CSolidObject* obj)
{
	if (obj == nullptr)
		return 0;

	return LuaUtils::PushColVolData(L, &obj->selectionVolume);
}





/******************************************************************************/
/******************************************************************************/
//
//  The call-outs
//

int LuaUnsyncedRead::IsReplay(lua_State* L)
{
	lua_pushboolean(L, gameSetup->hostDemo);
	return 1;
}


int LuaUnsyncedRead::GetReplayLength(lua_State* L)
{
	if (gameServer != nullptr && gameServer->GetDemoReader()) {
		lua_pushnumber(L, gameServer->GetDemoReader()->GetFileHeader().gameTime);
		return 1;
	}
	return 0;
}

/******************************************************************************/

int LuaUnsyncedRead::GetGameName(lua_State* L)
{
	lua_pushstring(L, modInfo.humanNameVersioned.c_str());
	return 1;
}

int LuaUnsyncedRead::GetMenuName(lua_State* L)
{
	lua_pushstring(L, luaMenuController->GetMenuName().c_str());
	return 1;
}


/******************************************************************************/

int LuaUnsyncedRead::GetProfilerTimeRecord(lua_State* L)
{
	const CTimeProfiler::TimeRecord& record = profiler.GetTimeRecord(lua_tostring(L, 1));

	lua_pushnumber(L, record.total.toMilliSecsf());
	lua_pushnumber(L, record.current.toMilliSecsf());
	// lua_pushnumber(L, record.frames[0].toMilliSecsf());
	lua_pushnumber(L, record.stats.x); // max-dt
	lua_pushnumber(L, record.stats.y); // time-%
	lua_pushnumber(L, record.stats.z); // peak-%
	return 5;
}

int LuaUnsyncedRead::GetProfilerRecordNames(lua_State* L)
{
	const auto& sortedProfiles = profiler.GetSortedProfiles();

	lua_createtable(L, sortedProfiles.size(), 0);

	for (size_t i = 0; i < sortedProfiles.size(); i++) {
		lua_pushnumber(L, i + 1); // key
		lua_pushsstring(L, sortedProfiles[i].first); // val
		lua_rawset(L, -3);
	}

	return 1;
}


int LuaUnsyncedRead::GetLuaMemUsage(lua_State* L)
{
	const luaContextData* lcd = GetLuaContextData(L);

	// handle and global stats
	const SLuaAllocState* lhs = &lcd->allocState;
	      SLuaAllocState  lgs;

	spring_lua_alloc_get_stats(&lgs);

	lua_pushnumber(L, lhs->allocedBytes / 1024.0f); // (kilo)bytes, can exceed 1<<24 otherwise
	lua_pushnumber(L, lhs->numLuaAllocs / 1000.0f); // (kilo)allocs, ditto
	lua_pushnumber(L, lgs.allocedBytes / 1024.0f);
	lua_pushnumber(L, lgs.numLuaAllocs / 1000.0f);

	// [0] := unsynced, [1] := synced
	extern const spring::unsynced_set<const luaContextData*>* LUAHANDLE_CONTEXTS[2];

	// sum up the individual (unsynced and synced) state footprints
	for (bool synced: {false, true}) {
		lgs.allocedBytes = {0};
		lgs.numLuaAllocs = {0};

		for (const luaContextData* lcd: *LUAHANDLE_CONTEXTS[synced]) {
			lhs = &lcd->allocState;

			lgs.allocedBytes += lhs->allocedBytes;
			lgs.numLuaAllocs += lhs->numLuaAllocs;
		}

		lua_pushnumber(L, lgs.allocedBytes / 1024.0f);
		lua_pushnumber(L, lgs.numLuaAllocs / 1000.0f);
	}

	return 8;
}

int LuaUnsyncedRead::GetVidMemUsage(lua_State* L)
{
	int2 vidMemInfo;

	GetAvailableVideoRAM(&vidMemInfo.x, globalRenderingInfo.glVendor);

	lua_pushnumber(L, (vidMemInfo.x - vidMemInfo.y) / 1024.0f); // MB (used = total - free)
	lua_pushnumber(L, (vidMemInfo.x               ) / 1024.0f); // MB
	return 2;
}


/******************************************************************************/

int LuaUnsyncedRead::GetViewGeometry(lua_State* L)
{
	lua_pushnumber(L, globalRendering->viewSizeX);
	lua_pushnumber(L, globalRendering->viewSizeY);
	lua_pushnumber(L, globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewPosY);
	return 4;
}


int LuaUnsyncedRead::GetWindowGeometry(lua_State* L)
{
	// origin BOTTOMLEFT
	const int winPosY_bl = globalRendering->screenSizeY - globalRendering->winSizeY - globalRendering->winPosY;

	lua_pushnumber(L, globalRendering->winSizeX);
	lua_pushnumber(L, globalRendering->winSizeY);
	lua_pushnumber(L, globalRendering->winPosX);
	lua_pushnumber(L, winPosY_bl);
	return 4;
}


int LuaUnsyncedRead::GetScreenGeometry(lua_State* L)
{
	lua_pushnumber(L, globalRendering->screenSizeX);
	lua_pushnumber(L, globalRendering->screenSizeY);
	lua_pushnumber(L, 0.0f);
	lua_pushnumber(L, 0.0f);
	return 4;
}


int LuaUnsyncedRead::GetMiniMapGeometry(lua_State* L)
{
	if (minimap == nullptr)
		return 0;

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
	if (minimap == nullptr)
		return 0;

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
	if (minimap == nullptr)
		return 0;

	if (minimap->GetMinimized() || game->hideInterface)
		return false;

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
	lua_pushnumber(L, globalRendering->drawFrame & 0xFFFF);
	lua_pushnumber(L, (globalRendering->drawFrame >> 16) & 0xFFFF);
	return 2;
}


int LuaUnsyncedRead::GetFrameTimeOffset(lua_State* L)
{
	lua_pushnumber(L, globalRendering->timeOffset);
	return 1;
}


int LuaUnsyncedRead::GetLastUpdateSeconds(lua_State* L)
{
	lua_pushnumber(L, game->updateDeltaSeconds);
	return 1;
}


int LuaUnsyncedRead::GetVideoCapturingMode(lua_State* L)
{
	lua_pushboolean(L, videoCapturing->AllowRecord());
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
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	if (CLuaHandle::GetHandleReadAllyTeam(L) < 0) {
		// in this case handle has full-read access since unit != nullptr
		lua_pushboolean(L, unit->allyteam == gu->myAllyTeam);
	} else {
		lua_pushboolean(L, unit->allyteam == CLuaHandle::GetHandleReadAllyTeam(L));
	}

	return 1;
}


int LuaUnsyncedRead::IsUnitInView(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, camera->InView(unit->midPos, unit->radius));
	return 1;
}


int LuaUnsyncedRead::IsUnitVisible(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const float radius = luaL_optnumber(L, 2, unit->radius);
	const bool checkIcon = lua_toboolean(L, 3);

	const int readAllyTeam = CLuaHandle::GetHandleReadAllyTeam(L);

	if (readAllyTeam < 0) {
		if (!CLuaHandle::GetHandleFullRead(L)) {
			lua_pushboolean(L, false);
		} else {
			lua_pushboolean(L,
				(!checkIcon || !unit->isIcon) &&
				camera->InView(unit->midPos, radius));
		}
	}
	else {
		if ((unit->losStatus[readAllyTeam] & LOS_INLOS) == 0) {
			lua_pushboolean(L, false);
		} else {
			lua_pushboolean(L,
				(!checkIcon || !unit->isIcon) &&
				camera->InView(unit->midPos, radius));
		}
	}
	return 1;
}


int LuaUnsyncedRead::IsUnitIcon(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, unit->isIcon);
	return 1;
}


int LuaUnsyncedRead::IsUnitSelected(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);
	if (unit == nullptr)
		return 0;

	const auto& selUnits = selectedUnitsHandler.selectedUnits;
	lua_pushboolean(L, selUnits.find(unit->id) != selUnits.end());
	return 1;
}


int LuaUnsyncedRead::GetUnitLuaDraw(lua_State* L)
{
	return (GetSolidObjectLuaDraw(L, ParseUnit(L, __func__, 1)));
}

int LuaUnsyncedRead::GetUnitNoDraw(lua_State* L)
{
	return (GetSolidObjectNoDraw(L, ParseUnit(L, __func__, 1)));
}


int LuaUnsyncedRead::GetUnitNoMinimap(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, unit->noMinimap);
	return 1;
}

int LuaUnsyncedRead::GetUnitNoSelect(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	lua_pushboolean(L, unit->noSelect);
	return 1;
}

int LuaUnsyncedRead::GetUnitSelectionVolumeData(lua_State* L)
{
	return GetSolidObjectSelectionVolume(L, ParseUnit(L, __func__, 1));
}

int LuaUnsyncedRead::GetFeatureLuaDraw(lua_State* L)
{
	return (GetSolidObjectLuaDraw(L, ParseFeature(L, __func__, 1)));
}

int LuaUnsyncedRead::GetFeatureNoDraw(lua_State* L)
{
	return (GetSolidObjectNoDraw(L, ParseFeature(L, __func__, 1)));
}

int LuaUnsyncedRead::GetFeatureSelectionVolumeData(lua_State* L)
{
	return GetSolidObjectSelectionVolume(L, ParseFeature(L, __func__, 1));
}



static int GetObjectTransformMatrix(const CSolidObject* o, lua_State* L)
{
	if (o == nullptr)
		return 0;

	CMatrix44f m = o->GetTransformMatrix(false);

	if (luaL_optboolean(L, 2, false))
		m = m.InvertAffine();

	for (int i = 0; i < 16; i += 4) {
		lua_pushnumber(L, m[i + 0]);
		lua_pushnumber(L, m[i + 1]);
		lua_pushnumber(L, m[i + 2]);
		lua_pushnumber(L, m[i + 3]);
	}

	return 16;
}

int LuaUnsyncedRead::GetUnitTransformMatrix(lua_State* L) { return (GetObjectTransformMatrix(ParseUnit(L, __func__, 1), L)); }
int LuaUnsyncedRead::GetFeatureTransformMatrix(lua_State* L) { return (GetObjectTransformMatrix(ParseFeature(L, __func__, 1), L)); }


static int GetObjectPieceTransformMatrices(CSolidObject* o, lua_State* L)
{
	if (o == nullptr)
		return 0;

	// NB: might be stale if object was not rendered for a while
	const LocalModel& lm = o->localModel;
	const std::vector<CMatrix44f>& pms = lm.GetPieceMatrices();

	// matrices = {[1] = matrix{...}, [2] = matrix{...}, ...}
	lua_createtable(L, pms.size(), 0);

	for (size_t i = 0, n = pms.size(); i < n; i++) {
		lua_pushnumber(L, i + 1);
		lua_createtable(L, 16, 0);

		for (int j = 0; j < 16; j += 4) {
			lua_pushnumber(L, j + 1); lua_pushnumber(L, pms[i][j + 0]); lua_rawset(L, -3);
			lua_pushnumber(L, j + 2); lua_pushnumber(L, pms[i][j + 1]); lua_rawset(L, -3);
			lua_pushnumber(L, j + 3); lua_pushnumber(L, pms[i][j + 2]); lua_rawset(L, -3);
			lua_pushnumber(L, j + 4); lua_pushnumber(L, pms[i][j + 3]); lua_rawset(L, -3);
		}

		lua_rawset(L, -3);
	}

	return 1;
}

int LuaUnsyncedRead::GetUnitPieceTransformMatrices(lua_State* L) { return (GetObjectPieceTransformMatrices(ParseUnit(L, __func__, 1), L)); }
int LuaUnsyncedRead::GetFeaturePieceTransformMatrices(lua_State* L) { return (GetObjectPieceTransformMatrices(ParseUnit(L, __func__, 1), L)); }



int LuaUnsyncedRead::GetUnitViewPosition(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;

	const float3 unitPos = (luaL_optboolean(L, 2, false)) ? unit->GetObjDrawMidPos() : unit->drawPos;
	const float3 errorVec = unit->GetLuaErrorVector(CLuaHandle::GetHandleReadAllyTeam(L), CLuaHandle::GetHandleFullRead(L));

	lua_pushnumber(L, unitPos.x + errorVec.x);
	lua_pushnumber(L, unitPos.y + errorVec.y);
	lua_pushnumber(L, unitPos.z + errorVec.z);
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


// never instantiated directly
template<class T> class CWorldObjectQuadDrawer: public CReadMap::IQuadDrawer {
public:
	typedef std::vector<T*> ObjectList;
	typedef std::vector< const ObjectList* > ObjectVector;

	void ResetState() override {
		objectLists.clear();
		objectLists.reserve(64);

		objectCount = 0;
	}

	unsigned int GetQuadCount() const { return (objectLists.size()); }
	unsigned int GetObjectCount() const { return objectCount; }

	const ObjectVector& GetObjectLists() { return objectLists; }

	void AddObjectList(const ObjectList* objects) {
		if (objects->empty())
			return;

		objectLists.push_back(objects);
		objectCount += objects->size();
	}

protected:
	// note: stores pointers to lists, not copies
	// its size equals the number of visible quads
	ObjectVector objectLists;

	unsigned int objectCount;
};


class CVisUnitQuadDrawer: public CWorldObjectQuadDrawer<CUnit> {
public:
	void DrawQuad(int x, int y) override {
		const CQuadField::Quad& q = quadField.GetQuadAt(x, y);
		const ObjectList* o = &q.units;

		AddObjectList(o);
	}
};

class CVisFeatureQuadDrawer: public CWorldObjectQuadDrawer<CFeature> {
public:
	void DrawQuad(int x, int y) override {
		const CQuadField::Quad& q = quadField.GetQuadAt(x, y);
		const ObjectList* o = &q.features;

		AddObjectList(o);
	}
};

class CVisProjectileQuadDrawer: public CWorldObjectQuadDrawer<CProjectile> {
public:
	void DrawQuad(int x, int y) override {
		const CQuadField::Quad& q = quadField.GetQuadAt(x, y);
		const ObjectList* o = &q.projectiles;

		AddObjectList(o);
	}
};




int LuaUnsyncedRead::GetVisibleUnits(lua_State* L)
{
	// arg 1 - teamID
	int teamID = luaL_optint(L, 1, -1);
	int allyTeamID = CLuaHandle::GetHandleReadAllyTeam(L);

	if (teamID == MyUnits) {
		const int scriptTeamID = CLuaHandle::GetHandleReadTeam(L);

		if (scriptTeamID >= 0) {
			teamID = scriptTeamID;
		} else {
			teamID = AllUnits;
		}
	}

	if (teamID >= 0) {
		if (!teamHandler.IsValidTeam(teamID))
			return 0;

		allyTeamID = teamHandler.AllyTeam(teamID);
	}
	if (allyTeamID < 0) {
		if (!CLuaHandle::GetHandleFullRead(L)) {
			return 0;
		}
	}

	// arg 3 - noIcons
	const bool noIcons = !luaL_optboolean(L, 3, true);

	float radiusMult = 1.0f;
	float testRadius = 0.0f;

	// arg 2 - use fixed test-value or add unit radii to it
	if (lua_israwnumber(L, 2)) {
		radiusMult = float((testRadius = lua_tofloat(L, 2)) >= 0.0f);
		testRadius = std::max(testRadius, -testRadius);
	}

	static CVisUnitQuadDrawer unitQuadIter;

	unitQuadIter.ResetState();
	readMap->GridVisibility(nullptr, &unitQuadIter, 1e9, CQuadField::BASE_QUAD_SIZE / SQUARE_SIZE);

	// Even though we're in unsynced it's ok to use gs->tempNum since its exact value
	// doesn't matter
	const int tempNum = gs->GetTempNum();
	lua_createtable(L, unitQuadIter.GetObjectCount(), 0);

	unsigned int count = 0;
	for (auto visUnitList: unitQuadIter.GetObjectLists()) {
		for (CUnit* u: *visUnitList) {
			if (u->tempNum == tempNum)
				continue;

			u->tempNum = tempNum;

			if (u->noDraw)
				continue;

			if (allyTeamID >= 0 && !(u->losStatus[allyTeamID] & LOS_INLOS))
				continue;

			if (noIcons && u->isIcon)
				continue;

			if ((teamID == AllyUnits)  && (allyTeamID != u->allyteam))
				continue;

			if ((teamID == EnemyUnits) && (allyTeamID == u->allyteam))
				continue;

			if ((teamID >= 0) && (teamID != u->team))
				continue;

			//No check for AllUnits, since there's no need.

			if (!camera->InView(u->drawMidPos, testRadius + (u->GetDrawRadius() * radiusMult)))
				continue;

			lua_pushnumber(L, u->id);
			lua_rawseti(L, -2, ++count);
		}
	}

	return 1;
}


int LuaUnsyncedRead::GetVisibleFeatures(lua_State* L)
{
	// arg 1 - allyTeamID
	int allyTeamID = luaL_optint(L, 1, -1);

	if (allyTeamID >= 0) {
		if (!teamHandler.ValidAllyTeam(allyTeamID)) {
			return 0;
		}
	} else {
		allyTeamID = -1;

		if (!CLuaHandle::GetHandleFullRead(L)) {
			allyTeamID = CLuaHandle::GetHandleReadAllyTeam(L);
		}
	}

	const bool noIcons = !luaL_optboolean(L, 3, true);
	const bool noGeos = !luaL_optboolean(L, 4, true);

	float radiusMult = 0.0f; // 0 or 1
	float testRadius = 0.0f;

	// arg 2 - use fixed test-value or add feature radii to it
	if (lua_israwnumber(L, 2)) {
		radiusMult = float((testRadius = lua_tofloat(L, 2)) >= 0.0f);
		testRadius = std::max(testRadius, -testRadius);
	}

	static CVisFeatureQuadDrawer featureQuadIter;

	featureQuadIter.ResetState();
	readMap->GridVisibility(nullptr, &featureQuadIter, 1e9, CQuadField::BASE_QUAD_SIZE / SQUARE_SIZE);

	// Even though we're in unsynced it's ok to use gs->tempNum since its exact value
	// doesn't matter
	const int tempNum = gs->GetTempNum();
	lua_createtable(L, featureQuadIter.GetObjectCount(), 0);

	unsigned int count = 0;
	for (auto visFeatureList: featureQuadIter.GetObjectLists()) {
		for (CFeature* f: *visFeatureList) {
			if (f->tempNum == tempNum)
				continue;

			f->tempNum = tempNum;

			if (f->noDraw)
				continue;

			if (noIcons && f->drawFlag == CFeature::FD_FARTEX_FLAG)
				continue;

			if (noGeos && f->def->geoThermal)
				continue;

			if (!gu->spectatingFullView && !f->IsInLosForAllyTeam(allyTeamID))
				continue;

			if (!camera->InView(f->drawMidPos, testRadius + (f->GetDrawRadius() * radiusMult)))
				continue;

			lua_pushnumber(L, f->id);
			lua_rawseti(L, -2, ++count);
		}
	}


	return 1;
}

int LuaUnsyncedRead::GetVisibleProjectiles(lua_State* L)
{
	int allyTeamID = luaL_optint(L, 1, -1);

	if (allyTeamID >= 0) {
		if (!teamHandler.ValidAllyTeam(allyTeamID)) {
			return 0;
		}
	} else {
		allyTeamID = -1;

		if (!CLuaHandle::GetHandleFullRead(L)) {
			allyTeamID = CLuaHandle::GetHandleReadAllyTeam(L);
		}
	}

	static CVisProjectileQuadDrawer projQuadIter;

	/*const bool addSyncedProjectiles =*/ luaL_optboolean(L, 2, true);
	const bool addWeaponProjectiles = luaL_optboolean(L, 3, true);
	const bool addPieceProjectiles = luaL_optboolean(L, 4, true);


	projQuadIter.ResetState();
	readMap->GridVisibility(nullptr, &projQuadIter, 1e9, CQuadField::BASE_QUAD_SIZE / SQUARE_SIZE);

	// Even though we're in unsynced it's ok to use gs->tempNum since its exact value
	// doesn't matter
	const int tempNum = gs->GetTempNum();
	lua_createtable(L, projQuadIter.GetObjectCount(), 0);

	unsigned int count = 0;
	for (auto visProjectileList: projQuadIter.GetObjectLists()) {
		for (CProjectile* p: *visProjectileList) {
			if (p->tempNum == tempNum)
				continue;

			p->tempNum = tempNum;


			if (allyTeamID >= 0 && !losHandler->InLos(p, allyTeamID))
				continue;

			if (!camera->InView(p->pos, p->GetDrawRadius()))
				continue;

			#if 1
			// filter out unsynced projectiles, the SyncedRead
			// projecile Get* functions accept only synced ID's
			// (specifically they interpret all ID's as synced)
			if (!p->synced)
				continue;
			#else
			if (!addSyncedProjectiles && p->synced)
				continue;
			#endif
			if (!addWeaponProjectiles && p->weapon)
				continue;
			if (!addPieceProjectiles && p->piece)
				continue;

			lua_pushnumber(L, p->id);
			lua_rawseti(L, -2, ++count);
		}
	}

	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetLocalPlayerID(lua_State* L)
{
	lua_pushnumber(L, gu->myPlayerNum);
	return 1;
}


int LuaUnsyncedRead::GetLocalTeamID(lua_State* L)
{
	lua_pushnumber(L, gu->myTeam);
	return 1;
}


int LuaUnsyncedRead::GetLocalAllyTeamID(lua_State* L)
{
	lua_pushnumber(L, gu->myAllyTeam);
	return 1;
}


int LuaUnsyncedRead::GetSpectatingState(lua_State* L)
{
	lua_pushboolean(L, gu->spectating);
	lua_pushboolean(L, gu->spectatingFullView);
	lua_pushboolean(L, gu->spectatingFullSelect);
	return 3;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetSelectedUnits(lua_State* L)
{
	unsigned int count = 0;
	const auto& selUnits = selectedUnitsHandler.selectedUnits;

	// { [1] = number unitID, ... }
	lua_createtable(L, selUnits.size(), 0);

	for (const int unitID: selUnits) {
		lua_pushnumber(L, unitID);
		lua_rawseti(L, -2, ++count);
	}
	return 1;
}


static std::vector< std::pair<int, std::vector<const CUnit*> > > gsusUnitDefMap;
static std::vector< std::pair<int, int> > gsucCountMap;

static std::vector< std::pair<int, std::vector<const CUnit*> > > ggusUnitDefMap;
static std::vector< std::pair<int, int> > ggucCountMap;


int LuaUnsyncedRead::GetSelectedUnitsSorted(lua_State* L)
{
	gsusUnitDefMap.clear();
	gsusUnitDefMap.resize(unitDefHandler->NumUnitDefs() + 1);

	int numDefKeys = 0;

	for (const int unitID: selectedUnitsHandler.selectedUnits) {
		const CUnit* unit = unitHandler.GetUnit(unitID);
		const UnitDef* unitDef = unit->unitDef;

		gsusUnitDefMap[unitDef->id].first = unitDef->id;
		gsusUnitDefMap[unitDef->id].second.push_back(unit);
	}

	// { [number unitDefID] = { [1] = [number unitID], ...}, ... }
	lua_createtable(L, 0, gsusUnitDefMap.size());

	for (const std::pair<int, std::vector<const CUnit*> >& p: gsusUnitDefMap) {
		const std::vector<const CUnit*>& v = p.second;

		if (v.empty())
			continue;

		{
			// inner array-table
			lua_createtable(L, v.size(), 0);

			for (unsigned int i = 0; i < v.size(); i++) {
				lua_pushnumber(L, v[i]->id);
				lua_rawseti(L, -2, i + 1);
			}

			// push the UnitDef index
			lua_rawseti(L, -2, p.first);
		}

		numDefKeys += 1;
	}

	// UnitDef ID keys are not necessarily consecutive
	HSTR_PUSH_NUMBER(L, "n", numDefKeys);
	return 1;
}


int LuaUnsyncedRead::GetSelectedUnitsCounts(lua_State* L)
{
	gsucCountMap.clear();
	gsucCountMap.resize(unitDefHandler->NumUnitDefs() + 1, {0, 0});

	int numDefKeys = 0;

	// tally the types
	for (const int unitID: selectedUnitsHandler.selectedUnits) {
		const CUnit* unit = unitHandler.GetUnit(unitID);
		const UnitDef* unitDef = unit->unitDef;

		gsucCountMap[unitDef->id].first = unitDef->id;
		gsucCountMap[unitDef->id].second += 1;
	}

	// { [number unitDefID] = number count, ... }
	lua_createtable(L, 0, gsucCountMap.size());

	for (const std::pair<int, int>& p: gsucCountMap) {
		if (p.second == 0)
			continue;

		lua_pushnumber(L, p.second); // push the UnitDef unit count (value)
		lua_rawseti(L, -2, p.first); // push the UnitDef index (key)

		numDefKeys += 1;
	}

	// UnitDef ID keys are not necessarily consecutive
	HSTR_PUSH_NUMBER(L, "n", numDefKeys);
	return 1;
}


int LuaUnsyncedRead::GetSelectedUnitsCount(lua_State* L)
{
	lua_pushnumber(L, selectedUnitsHandler.selectedUnits.size());
	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::IsGUIHidden(lua_State* L)
{
	lua_pushboolean(L, game != nullptr && game->hideInterface);
	return 1;
}


int LuaUnsyncedRead::HaveShadows(lua_State* L)
{
	lua_pushboolean(L, shadowHandler.ShadowsLoaded());
	return 1;
}


int LuaUnsyncedRead::HaveAdvShading(lua_State* L)
{
	lua_pushboolean(L, true);
	return 1;
}


int LuaUnsyncedRead::GetWaterMode(lua_State* L)
{
	lua_pushnumber(L, water->GetID());
	lua_pushstring(L, water->GetName());
	return 2;
}


int LuaUnsyncedRead::GetMapDrawMode(lua_State* L)
{
	using P = std::pair<const char*, const char*>;
	constexpr std::array<P, 5> modes = {
		P(        "", "normal"            ),
		P(    "path", "pathTraversability"),
		P(    "heat", "pathHeat"          ),
		P(    "flow", "pathFlow"          ),
		P("pathcost", "pathCost"          ),
	};

	const auto& mode = infoTextureHandler->GetMode();
	const auto  iter = std::find_if(modes.begin(), modes.end(), [&mode](const P& p) { return (strcmp(p.first, mode.c_str()) == 0); });

	if (iter != modes.end()) {
		lua_pushstring(L, iter->second);
	} else {
		lua_pushstring(L, mode.c_str());
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

	CBaseGroundDrawer* groundDrawer = readMap->GetGroundDrawer();
	CBaseGroundTextures* groundTextures = groundDrawer->GetGroundTextures();

	if (groundTextures == nullptr) {
		lua_pushboolean(L, false);
		return 1;
	}
	if (texName.empty()) {
		lua_pushboolean(L, false);
		return 1;
	}

	const LuaTextures& luaTextures = CLuaHandle::GetActiveTextures(L);
	const LuaTextures::Texture* luaTexture = luaTextures.GetInfo(texName);

	if (luaTexture == nullptr) {
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

int LuaUnsyncedRead::GetLosViewColors(lua_State* L)
{
#define PACK_COLOR_VECTOR(color) \
	lua_createtable(L, 3, 0); \
	lua_pushnumber(L, color[0] / scale); lua_rawseti(L, -2, 1); \
	lua_pushnumber(L, color[1] / scale); lua_rawseti(L, -2, 2); \
	lua_pushnumber(L, color[2] / scale); lua_rawseti(L, -2, 3);

	const float scale = (float)CBaseGroundDrawer::losColorScale;
	CBaseGroundDrawer* gd = readMap->GetGroundDrawer();

	PACK_COLOR_VECTOR(gd->alwaysColor);
	PACK_COLOR_VECTOR(gd->losColor);
	PACK_COLOR_VECTOR(gd->radarColor);
	PACK_COLOR_VECTOR(gd->jamColor);
	PACK_COLOR_VECTOR(gd->radarColor2);
	return 5;
}


/******************************************************************************/

int LuaUnsyncedRead::GetCameraNames(lua_State* L)
{
	const std::array<CCameraController*, CCameraHandler::CAMERA_MODE_LAST>& cc = camHandler->GetControllers();

	lua_createtable(L, 0, cc.size());

	for (size_t i = 0; i < cc.size(); ++i) {
		lua_pushsstring(L, cc[i]->GetName()); // key
		lua_pushnumber(L, i); // val
		lua_rawset(L, -3);
	}

	return 1;
}

int LuaUnsyncedRead::GetCameraState(lua_State* L)
{
	CCameraController::StateMap camState;
	camHandler->GetState(camState);

	if (!luaL_optboolean(L, 1, true)) {
		// table-less version; pushes just the name and the cam-specific values
		// use GetCamera{Position,Direction,FOV} for the common fields from the base class
		lua_pushsstring(L, camHandler->GetCurrentController().GetName());

		switch (camHandler->GetCurrentControllerNum()) {
		case CCameraHandler::CAMERA_MODE_FIRSTPERSON:
		case CCameraHandler::CAMERA_MODE_ROTOVERHEAD: // happens to have the same set of values as FPS
			lua_pushnumber(L, camState["oldHeight"]);
			return 1 + 1;

		case CCameraHandler::CAMERA_MODE_OVERHEAD:
			lua_pushnumber(L, camState["height"]);
			lua_pushnumber(L, camState["angle"]);
			lua_pushnumber(L, camState["flipped"]);
			return 1 + 3;

		case CCameraHandler::CAMERA_MODE_SPRING:
			lua_pushnumber(L, camState["rx"]);
			lua_pushnumber(L, camState["ry"]);
			lua_pushnumber(L, camState["rz"]);
			lua_pushnumber(L, camState["dist"]);
			return 1 + 4;

		case CCameraHandler::CAMERA_MODE_FREE:
			lua_pushnumber(L, camState["rx"]);
			lua_pushnumber(L, camState["ry"]);
			lua_pushnumber(L, camState["rz"]);
			lua_pushnumber(L, camState["vx"]);
			lua_pushnumber(L, camState["vy"]);
			lua_pushnumber(L, camState["vz"]);
			lua_pushnumber(L, camState["avx"]);
			lua_pushnumber(L, camState["avy"]);
			lua_pushnumber(L, camState["avz"]);
			return 1 + 9;

		case CCameraHandler::CAMERA_MODE_OVERVIEW:
		default:
			// overview has no extra values
			return 1 + 0;
		}
	}

	lua_newtable(L);

	lua_pushliteral(L, "name");
	lua_pushsstring(L, (camHandler->GetCurrentController()).GetName());
	lua_rawset(L, -3);

	for (const auto& s: camState) {
		lua_pushsstring(L, s.first);
		lua_pushnumber(L, s.second);
		lua_rawset(L, -3);
	}

	return 1;
}


int LuaUnsyncedRead::GetCameraPosition(lua_State* L)
{
	lua_pushnumber(L, camera->GetPos().x);
	lua_pushnumber(L, camera->GetPos().y);
	lua_pushnumber(L, camera->GetPos().z);
	return 3;
}

int LuaUnsyncedRead::GetCameraDirection(lua_State* L)
{
	lua_pushnumber(L, camera->GetDir().x);
	lua_pushnumber(L, camera->GetDir().y);
	lua_pushnumber(L, camera->GetDir().z);
	return 3;
}

int LuaUnsyncedRead::GetCameraFOV(lua_State* L)
{
	lua_pushnumber(L, camera->GetVFOV());
	lua_pushnumber(L, camera->GetHFOV());
	return 2;
}

int LuaUnsyncedRead::GetCameraVectors(lua_State* L)
{
#define PACK_CAMERA_VECTOR(s,n) \
	HSTR_PUSH(L, #s);           \
	lua_createtable(L, 3, 0);            \
	lua_pushnumber(L, camera-> n .x); lua_rawseti(L, -2, 1); \
	lua_pushnumber(L, camera-> n .y); lua_rawseti(L, -2, 2); \
	lua_pushnumber(L, camera-> n .z); lua_rawseti(L, -2, 3); \
	lua_rawset(L, -3)

	lua_newtable(L);
	PACK_CAMERA_VECTOR(forward, GetDir());
	PACK_CAMERA_VECTOR(up, GetUp());
	PACK_CAMERA_VECTOR(right, GetRight());
	PACK_CAMERA_VECTOR(topFrustumPlane, GetFrustumPlane(CCamera::FRUSTUM_PLANE_TOP));
	PACK_CAMERA_VECTOR(botFrustumPlane, GetFrustumPlane(CCamera::FRUSTUM_PLANE_BOT));
	PACK_CAMERA_VECTOR(lftFrustumPlane, GetFrustumPlane(CCamera::FRUSTUM_PLANE_LFT));
	PACK_CAMERA_VECTOR(rgtFrustumPlane, GetFrustumPlane(CCamera::FRUSTUM_PLANE_RGT));

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

	const int wx =                                  mx + globalRendering->viewPosX;
	const int wy = globalRendering->viewSizeY - 1 - my - globalRendering->viewPosY;

	const int optArgIdx = 3 + lua_isnumber(L, 3); // 3 or 4
	const int newArgIdx = 3 + 4 * (optArgIdx == 3); // 7 or 3

	const bool onlyCoords  = luaL_optboolean(L, optArgIdx + 0, false);
	const bool useMiniMap  = luaL_optboolean(L, optArgIdx + 1, false);
	const bool includeSky  = luaL_optboolean(L, optArgIdx + 2, false);
	const bool ignoreWater = luaL_optboolean(L, optArgIdx + 3, false);

	if (useMiniMap && (minimap != nullptr) && !minimap->GetMinimized()) {
		const int px = minimap->GetPosX() - globalRendering->viewPosX; // for left dualscreen
		const int py = minimap->GetPosY();
		const int sx = minimap->GetSizeX();
		const int sy = minimap->GetSizeY();

		if ((mx >= px) && (mx < (px + sx))  &&  (my >= py) && (my < (py + sy))) {
			const float3 pos = minimap->GetMapPosition(wx, wy);

			if (!onlyCoords) {
				const CUnit* unit = minimap->GetSelectUnit(pos);

				if (unit != nullptr) {
					lua_pushliteral(L, "unit");
					lua_pushnumber(L, unit->id);
					return 2;
				}
			}

			lua_pushliteral(L, "ground");
			lua_createtable(L, 3, 0);
			lua_pushnumber(L,                                      pos.x ); lua_rawseti(L, -2, 1);
			lua_pushnumber(L, CGround::GetHeightReal(pos.x, pos.z, false)); lua_rawseti(L, -2, 2);
			lua_pushnumber(L,                                      pos.z ); lua_rawseti(L, -2, 3);
			return 2;
		}
	}

	if ((mx < 0) || (mx >= globalRendering->viewSizeX))
		return 0;
	if ((my < 0) || (my >= globalRendering->viewSizeY))
		return 0;

	const CUnit* unit = nullptr;
	const CFeature* feature = nullptr;

	const float rawRange = camera->GetFarPlaneDist() * 1.4f;
	const float badRange = rawRange - 300.0f;

	const float3 camPos = camera->GetPos();
	const float3 pxlDir = camera->CalcPixelDir(wx, wy);

	// trace for player's allyteam
	const float traceDist = TraceRay::GuiTraceRay(camPos, pxlDir, rawRange, nullptr, unit, feature, true, onlyCoords, ignoreWater);
	const float planeDist = CGround::LinePlaneCol(camPos, pxlDir, rawRange, luaL_optnumber(L, newArgIdx, 0.0f));

	const float3 tracePos = camPos + (pxlDir * traceDist);
	const float3 planePos = camPos + (pxlDir * planeDist); // backup (for includeSky and onlyCoords)

	if ((traceDist < 0.0f || traceDist > badRange) && unit == nullptr && feature == nullptr) {
		// ray went into the void (or started too far above terrain)
		if (!includeSky)
			return 0;

		lua_pushliteral(L, "sky");
	} else {
		if (!onlyCoords) {
			if (unit != nullptr) {
				lua_pushliteral(L, "unit");
				lua_pushnumber(L, unit->id);
				return 2;
			}

			if (feature != nullptr) {
				lua_pushliteral(L, "feature");
				lua_pushnumber(L, feature->id);
				return 2;
			}
		}

		lua_pushliteral(L, "ground");
	}

	lua_createtable(L, 6, 0);
	lua_pushnumber(L, tracePos.x); lua_rawseti(L, -2, 1);
	lua_pushnumber(L, tracePos.y); lua_rawseti(L, -2, 2);
	lua_pushnumber(L, tracePos.z); lua_rawseti(L, -2, 3);
	lua_pushnumber(L, planePos.x); lua_rawseti(L, -2, 4);
	lua_pushnumber(L, planePos.y); lua_rawseti(L, -2, 5);
	lua_pushnumber(L, planePos.z); lua_rawseti(L, -2, 6);

	return 2;
}


int LuaUnsyncedRead::GetPixelDir(lua_State* L)
{
	const int x = luaL_checkint(L, 1);
	const int y = luaL_checkint(L, 2);
	const float3 dir = camera->CalcPixelDir(x, y);
	lua_pushnumber(L, dir.x);
	lua_pushnumber(L, dir.y);
	lua_pushnumber(L, dir.z);
	return 3;
}



/******************************************************************************/

static bool AddPlayerToRoster(lua_State* L, int playerID, bool onlyActivePlayers, bool includePathingFlag)
{
#define PUSH_ROSTER_ENTRY(type, val) \
	lua_push ## type(L, val); lua_rawseti(L, -2, index++);

	const CPlayer* p = playerHandler.Player(playerID);

	if (onlyActivePlayers && !p->active)
		return false;

	int index = 1;
	lua_newtable(L);
	PUSH_ROSTER_ENTRY(string, p->name.c_str());
	PUSH_ROSTER_ENTRY(number, playerID);
	PUSH_ROSTER_ENTRY(number, p->team);
	PUSH_ROSTER_ENTRY(number, teamHandler.AllyTeam(p->team));
	PUSH_ROSTER_ENTRY(boolean, p->spectator);
	PUSH_ROSTER_ENTRY(number, p->cpuUsage);

	if (!includePathingFlag || p->ping != PATHING_FLAG) {
		const float pingSecs = p->ping * 0.001f;
		PUSH_ROSTER_ENTRY(number, pingSecs);
	} else {
		const float pingSecs = float(p->ping);
		PUSH_ROSTER_ENTRY(number, pingSecs);
	}

	return true;
}


int LuaUnsyncedRead::GetTeamColor(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);
	if ((teamID < 0) || (teamID >= teamHandler.ActiveTeams()))
		return 0;

	const CTeam* team = teamHandler.Team(teamID);
	if (team == nullptr)
		return 0;

	lua_pushnumber(L, team->color[0] / 255.0f);
	lua_pushnumber(L, team->color[1] / 255.0f);
	lua_pushnumber(L, team->color[2] / 255.0f);
	lua_pushnumber(L, team->color[3] / 255.0f);
	return 4;
}


int LuaUnsyncedRead::GetTeamOrigColor(lua_State* L)
{
	const int teamID = luaL_checkint(L, 1);
	if ((teamID < 0) || (teamID >= teamHandler.ActiveTeams()))
		return 0;

	const CTeam* team = teamHandler.Team(teamID);
	if (team == nullptr)
		return 0;

	lua_pushnumber(L, team->origColor[0] / 255.0f);
	lua_pushnumber(L, team->origColor[1] / 255.0f);
	lua_pushnumber(L, team->origColor[2] / 255.0f);
	lua_pushnumber(L, team->origColor[3] / 255.0f);
	return 4;
}


/******************************************************************************/
/******************************************************************************/

static void PushTimer(lua_State* L, const spring_time& time)
{
	// use time since Spring's epoch in MILLIseconds because that
	// is more likely to fit in a 32-bit pointer (on any platforms
	// where sizeof(void*) == 4) than time since ::chrono's epoch
	// (which can be arbitrarily large) and can be represented by
	// single-precision floats better
	//
	// 4e9millis == 4e6s == 46.3 days until overflow
	const std::uint64_t millis = time.toMilliSecs<std::uint64_t>();

	ptrdiff_t p = 0;

	if (sizeof(void*) == 8) {
		*reinterpret_cast<std::uint64_t*>(&p) = millis;
	} else {
		*reinterpret_cast<std::uint32_t*>(&p) = millis;
	}

	lua_pushlightuserdata(L, reinterpret_cast<void*>(p));
}

int LuaUnsyncedRead::GetTimer(lua_State* L)
{
	PushTimer(L, spring_now());
	return 1;
}

int LuaUnsyncedRead::GetFrameTimer(lua_State* L)
{
	if (luaL_optboolean(L, 1, false)) {
		PushTimer(L, game->lastFrameTime);
	} else {
		PushTimer(L, globalRendering->lastFrameStart);
	}
	return 1;
}


int LuaUnsyncedRead::DiffTimers(lua_State* L)
{
	if (!lua_islightuserdata(L, 1) || !lua_islightuserdata(L, 2)) {
		luaL_error(L, "Incorrect arguments to DiffTimers()");
	}

	const void* p1 = lua_touserdata(L, 1);
	const void* p2 = lua_touserdata(L, 2);

	const std::uint64_t t1 = (sizeof(void*) == 8)?
		*reinterpret_cast<std::uint64_t*>(&p1):
		*reinterpret_cast<std::uint32_t*>(&p1);
	const std::uint64_t t2 = (sizeof(void*) == 8)?
		*reinterpret_cast<std::uint64_t*>(&p2):
		*reinterpret_cast<std::uint32_t*>(&p2);

	// t1 is supposed to be the most recent time-point
	assert(t1 >= t2);

	const spring_time dt = spring_time::fromMilliSecs(t1 - t2);

	if (luaL_optboolean(L, 3, false)) {
		lua_pushnumber(L, dt.toMilliSecsf());
	} else {
		lua_pushnumber(L, dt.toSecsf());
	}

	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetSoundStreamTime(lua_State* L)
{
	lua_pushnumber(L, Channels::BGMusic->StreamGetPlayTime());
	lua_pushnumber(L, Channels::BGMusic->StreamGetTime());
	return 2;
}


int LuaUnsyncedRead::GetSoundEffectParams(lua_State* L)
{
#if defined(HEADLESS) || defined(NO_SOUND)
	return 0;
#else
	if (!efx.Supported())
		return 0;

	EAXSfxProps& efxprops = efx.sfxProperties;

	lua_createtable(L, 0, 2);

	size_t n = efxprops.filter_props_f.size();

	lua_pushliteral(L, "passfilter");
	lua_createtable(L, 0, n);
	lua_rawset(L, -3);

	for (const auto& filterProp: efxprops.filter_props_f) {
		const ALuint param = filterProp.first;
		const auto fit = alFilterParamToName.find(param);

		if (fit != alFilterParamToName.end()) {
			lua_pushsstring(L, fit->second); // name
			lua_pushnumber(L, filterProp.second);
			lua_rawset(L, -3);
		}
	}


	n = efxprops.reverb_props_v.size() + efxprops.reverb_props_f.size() + efxprops.reverb_props_i.size();

	lua_pushliteral(L, "reverb");
	lua_createtable(L, 0, n);
	lua_rawset(L, -3);

	for (const auto& reverbProp: efxprops.reverb_props_f) {
		const ALuint param = reverbProp.first;
		const auto fit = alParamToName.find(param);

		if (fit != alParamToName.end()) {
			lua_pushsstring(L, fit->second); // name
			lua_pushnumber(L, reverbProp.second);
			lua_rawset(L, -3);
		}
	}
	for (const auto& reverbProp: efxprops.reverb_props_v) {
		const ALuint param = reverbProp.first;
		const auto fit = alParamToName.find(param);

		if (fit != alParamToName.end()) {
			const float3& v = reverbProp.second;

			lua_pushsstring(L, fit->second); // name
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
	for (const auto& reverbProp: efxprops.reverb_props_i) {
		const ALuint param = reverbProp.first;
		const auto fit = alParamToName.find(param);

		if (fit != alParamToName.end()) {
			lua_pushsstring(L, fit->second); // name
			lua_pushboolean(L, reverbProp.second);
			lua_rawset(L, -3);
		}
	}

	return 1;
#endif // defined(HEADLESS) || defined(NO_SOUND)
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
	assert(globalRendering != nullptr);
	// true FPS is never fractional, but the calculation averages over time
	lua_pushnumber(L, int(globalRendering->FPS));
	return 1;
}


int LuaUnsyncedRead::GetGameSpeed(lua_State* L)
{
	lua_pushnumber(L, gs->wantedSpeedFactor);
	lua_pushnumber(L, gs->speedFactor);
	lua_pushboolean(L, gs->paused);
	return 3;
}

int LuaUnsyncedRead::GetGameState(lua_State* L)
{
  const float maxLatency = luaL_optfloat(L, 1, 500.0f);

	lua_pushboolean(L, game->IsDoneLoading());
	lua_pushboolean(L, game->IsSavedGame());
	lua_pushboolean(L, game->IsClientPaused()); // local state; included for demos
	lua_pushboolean(L, game->IsSimLagging(maxLatency));
	return 4;
}


/******************************************************************************/

int LuaUnsyncedRead::GetActiveCommand(lua_State* L)
{
	if (guihandler == nullptr)
		return 0;

	const vector<SCommandDescription>& cmdDescs = guihandler->commands;
	const int cmdDescCount = (int)cmdDescs.size();

	const int inCommand = guihandler->inCommand;
	lua_pushnumber(L, inCommand + CMD_INDEX_OFFSET);

	if ((inCommand < 0) || (inCommand >= cmdDescCount))
		return 1;

	lua_pushnumber(L, cmdDescs[inCommand].id);
	lua_pushnumber(L, cmdDescs[inCommand].type);
	lua_pushsstring(L, cmdDescs[inCommand].name);
	return 4;
}


int LuaUnsyncedRead::GetDefaultCommand(lua_State* L)
{
	if (guihandler == nullptr)
		return 0;

	const int defCmd = guihandler->GetDefaultCommand(mouse->lastx, mouse->lasty);

	const vector<SCommandDescription>& cmdDescs = guihandler->commands;
	const int cmdDescCount = (int)cmdDescs.size();

	lua_pushnumber(L, defCmd + CMD_INDEX_OFFSET);

	if ((defCmd < 0) || (defCmd >= cmdDescCount))
		return 1;

	lua_pushnumber(L, cmdDescs[defCmd].id);
	lua_pushnumber(L, cmdDescs[defCmd].type);
	lua_pushsstring(L, cmdDescs[defCmd].name);
	return 4;
}


int LuaUnsyncedRead::GetActiveCmdDescs(lua_State* L)
{
	if (guihandler == nullptr)
		return 0;

	const vector<SCommandDescription>& cmdDescs = guihandler->commands;
	const int cmdDescCount = (int)cmdDescs.size();

	lua_checkstack(L, 1 + 2);
	lua_newtable(L);

	for (int i = 0; i < cmdDescCount; i++) {
		LuaUtils::PushCommandDesc(L, cmdDescs[i]);
		lua_rawseti(L, -2, i + CMD_INDEX_OFFSET);
	}
	return 1;
}


int LuaUnsyncedRead::GetActiveCmdDesc(lua_State* L)
{
	if (guihandler == nullptr)
		return 0;

	const int cmdIndex = luaL_checkint(L, 1) - CMD_INDEX_OFFSET;

	const vector<SCommandDescription>& cmdDescs = guihandler->commands;
	const int cmdDescCount = (int)cmdDescs.size();

	if ((cmdIndex < 0) || (cmdIndex >= cmdDescCount))
		return 0;

	LuaUtils::PushCommandDesc(L, cmdDescs[cmdIndex]);
	return 1;
}


int LuaUnsyncedRead::GetCmdDescIndex(lua_State* L)
{
	if (guihandler == nullptr)
		return 0;

	const int cmdId = luaL_checkint(L, 1);

	const vector<SCommandDescription>& cmdDescs = guihandler->commands;
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
	if (guihandler == nullptr)
		return 0;

	lua_pushnumber(L, guihandler->buildFacing);
	return 1;
}


int LuaUnsyncedRead::GetBuildSpacing(lua_State* L)
{
	if (guihandler == nullptr)
		return 0;

	lua_pushnumber(L, guihandler->buildSpacing);
	return 1;
}


int LuaUnsyncedRead::GetGatherMode(lua_State* L)
{
	if (guihandler == nullptr)
		return 0;

	lua_pushnumber(L, guihandler->GetGatherMode());
	return 1;
}


/******************************************************************************/

int LuaUnsyncedRead::GetActivePage(lua_State* L)
{
	if (guihandler == nullptr)
		return 0;

	lua_pushnumber(L, guihandler->GetActivePage());
	lua_pushnumber(L, guihandler->GetMaxPage());
	return 2;
}


/******************************************************************************/

int LuaUnsyncedRead::GetMouseState(lua_State* L)
{
	assert(mouse != nullptr);
	assert(globalRendering != nullptr);

	lua_pushnumber(L, mouse->lastx - globalRendering->viewPosX);
	lua_pushnumber(L, globalRendering->viewSizeY - mouse->lasty - 1);

	lua_pushboolean(L, mouse->buttons[SDL_BUTTON_LEFT  ].pressed);
	lua_pushboolean(L, mouse->buttons[SDL_BUTTON_MIDDLE].pressed);
	lua_pushboolean(L, mouse->buttons[SDL_BUTTON_RIGHT ].pressed);
	lua_pushboolean(L, mouse->offscreen);
	lua_pushboolean(L, mouse->mmbScroll);
	return 7;
}


int LuaUnsyncedRead::GetMouseCursor(lua_State* L)
{
	assert(mouse != nullptr);

	lua_pushsstring(L, mouse->GetCurrentCursor());
	lua_pushnumber(L, mouse->GetCurrentCursorScale());
	return 2;
}


int LuaUnsyncedRead::GetMouseStartPosition(lua_State* L)
{
	assert(mouse != nullptr);

	const int button = luaL_checkint(L, 1);

	if ((button <= 0) || (button > NUM_BUTTONS))
		return 0;

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

int LuaUnsyncedRead::GetClipboard(lua_State* L)
{
	char* text = SDL_GetClipboardText();
	if (text == nullptr)
		return 0;
	lua_pushstring(L, text);
	SDL_free(text);
	return 1;
}

/******************************************************************************/

int LuaUnsyncedRead::GetLastMessagePositions(lua_State* L)
{
	lua_createtable(L, infoConsole->GetMsgPosCount(), 0);

	for (unsigned int i = 1; i <= infoConsole->GetMsgPosCount(); i++) {
		lua_createtable(L, 3, 0); {
			const float3 msgpos = infoConsole->GetMsgPos();
			lua_pushnumber(L, msgpos.x); lua_rawseti(L, -2, 1);
			lua_pushnumber(L, msgpos.y); lua_rawseti(L, -2, 2);
			lua_pushnumber(L, msgpos.z); lua_rawseti(L, -2, 3);
		}
		lua_rawseti(L, -2, i);
	}

	return 1;
}

/******************************************************************************/

int LuaUnsyncedRead::GetConsoleBuffer(lua_State* L)
{
	std::vector<CInfoConsole::RawLine> lines;
	infoConsole->GetRawLines(lines);

	const size_t lineCount = lines.size();
	      size_t startLine = 0;

	if (lua_gettop(L) >= 1)
		startLine = lineCount - std::min(lineCount, size_t(luaL_checkint(L, 1)));

	// table = { [1] = { text = string, zone = number}, etc... }
	lua_createtable(L, lineCount - startLine, 0);

	for (size_t i = startLine, n = 0; i < lineCount; i++) {
		lua_pushnumber(L, ++n);
		lua_createtable(L, 0, 2); {
			lua_pushliteral(L, "text");
			lua_pushsstring(L, lines[i].text);
			lua_rawset(L, -3);
			lua_pushliteral(L, "priority");
			lua_pushnumber(L, lines[i].level);
			lua_rawset(L, -3);
		}
		lua_rawset(L, -3);
	}

	return 1;
}


int LuaUnsyncedRead::GetCurrentTooltip(lua_State* L)
{
	lua_pushsstring(L, mouse->GetCurrentTooltip());
	return 1;
}

int LuaUnsyncedRead::IsUserWriting(lua_State* L)
{
	lua_pushboolean(L, gameTextInput.userWriting);
	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetKeyState(lua_State* L)
{
	const int key = SDL12_keysyms(luaL_checkint(L, 1));
	lua_pushboolean(L, KeyInput::IsKeyPressed(key));
	return 1;
}


int LuaUnsyncedRead::GetModKeyState(lua_State* L)
{
	lua_pushboolean(L, KeyInput::GetKeyModState(KMOD_ALT));
	lua_pushboolean(L, KeyInput::GetKeyModState(KMOD_CTRL));
	lua_pushboolean(L, KeyInput::GetKeyModState(KMOD_GUI));
	lua_pushboolean(L, KeyInput::GetKeyModState(KMOD_SHIFT));
	return 4;
}


int LuaUnsyncedRead::GetPressedKeys(lua_State* L)
{
	const auto& keys = KeyInput::GetPressedKeys();

	lua_createtable(L, keys.size(), 0);

	for (auto key: keys) {
		if (!key.second)
			continue;

		const int keyCode = SDL21_keysyms(key.first);

		// [keyCode] = true
		lua_pushboolean(L, true);
		lua_rawseti(L, -2, keyCode);

		// ["keyName"] = true
		lua_pushsstring(L, keyCodes.GetName(keyCode));
		lua_pushboolean(L, true);
		lua_rawset(L, -3);
	}

	return 1;
}


int LuaUnsyncedRead::GetInvertQueueKey(lua_State* L)
{
	if (guihandler == nullptr)
		return 0;

	lua_pushboolean(L, guihandler->GetInvertQueueKey());
	return 1;
}


int LuaUnsyncedRead::GetKeyCode(lua_State* L)
{
	lua_pushnumber(L, SDL21_keysyms(keyCodes.GetCode(luaL_checksstring(L, 1))));
	return 1;
}


int LuaUnsyncedRead::GetKeySymbol(lua_State* L)
{
	const int keycode = SDL12_keysyms(luaL_checkint(L, 1));
	lua_pushsstring(L, keyCodes.GetName(keycode));
	lua_pushsstring(L, keyCodes.GetDefaultName(keycode));
	return 2;
}


int LuaUnsyncedRead::GetKeyBindings(lua_State* L)
{
	CKeySet ks;

	if (!ks.Parse(luaL_checksstring(L, 1)))
		return 0;

	const CKeyBindings::ActionList& actions = keyBindings.GetActionList(ks);

	int i = 1;
	lua_newtable(L);
	for (const Action& action: actions) {
		lua_newtable(L);
			lua_pushsstring(L, action.command);
			lua_pushsstring(L, action.extra);
			lua_rawset(L, -3);
			LuaPushNamedString(L, "command",   action.command);
			LuaPushNamedString(L, "extra",     action.extra);
			LuaPushNamedString(L, "boundWith", action.boundWith);
		lua_rawseti(L, -2, i++);
	}
	return 1;
}


int LuaUnsyncedRead::GetActionHotKeys(lua_State* L)
{
	const CKeyBindings::HotkeyList& hotkeys = keyBindings.GetHotkeys(luaL_checksstring(L, 1));

	lua_createtable(L, 0, hotkeys.size());
	int i = 1;
	for (const std::string& hotkey: hotkeys) {
		lua_pushsstring(L, hotkey);
		lua_rawseti(L, -2, i++);
	}
	return 1;
}

/******************************************************************************/

int LuaUnsyncedRead::GetGroupList(lua_State* L)
{
	unsigned int count = 0;

	const std::vector<CGroup>& groups = uiGroupHandlers[gu->myTeam].GetGroups();

	// not an array-table
	lua_createtable(L, 0, groups.size());

	for (const CGroup& group: groups) {
		if (group.units.empty())
			continue;

		lua_pushnumber(L, group.units.size());
		lua_rawseti(L, -2, group.id);
		count++;
	}

	lua_pushnumber(L, count);
	return 2;
}


int LuaUnsyncedRead::GetSelectedGroup(lua_State* L)
{
	lua_pushnumber(L, selectedUnitsHandler.GetSelectedGroup());
	return 1;
}


int LuaUnsyncedRead::GetUnitGroup(lua_State* L)
{
	const CUnit* unit = ParseUnit(L, __func__, 1);

	if (unit == nullptr)
		return 0;
	if (unit->team != gu->myTeam)
		return 0;

	const CGroup* group = unit->GetGroup();

	if (group == nullptr)
		return 0;

	lua_pushnumber(L, group->id);
	return 1;
}


/******************************************************************************/

int LuaUnsyncedRead::GetGroupUnits(lua_State* L)
{
	const int groupID = luaL_checkint(L, 1);

	if (!uiGroupHandlers[gu->myTeam].HasGroup(groupID))
		return 0; // nils

	const CGroup* group = uiGroupHandlers[gu->myTeam].GetGroup(groupID);

	lua_createtable(L, group->units.size(), 0);

	unsigned int count = 0;

	for (const int unitID: group->units) {
		lua_pushnumber(L, unitID);
		lua_rawseti(L, -2, ++count);
	}

	return 1;
}


int LuaUnsyncedRead::GetGroupUnitsSorted(lua_State* L)
{
	const int groupID = luaL_checkint(L, 1);

	if (!uiGroupHandlers[gu->myTeam].HasGroup(groupID))
		return 0; // nils

	const CGroup* group = uiGroupHandlers[gu->myTeam].GetGroup(groupID);

	ggusUnitDefMap.clear();
	ggusUnitDefMap.resize(unitDefHandler->NumUnitDefs() + 1);

	for (const int unitID: group->units) {
		const CUnit* unit = unitHandler.GetUnit(unitID);
		const UnitDef* unitDef = unit->unitDef;

		ggusUnitDefMap[unitDef->id].first = unitDef->id;
		ggusUnitDefMap[unitDef->id].second.push_back(unit);
	}

	lua_createtable(L, 0, ggusUnitDefMap.size());

	for (const auto& el: ggusUnitDefMap) {
		const std::vector<const CUnit*>& v = el.second;

		if (v.empty())
			continue;

		lua_pushnumber(L, el.first); // push the UnitDef index
		lua_createtable(L, v.size(), 0); {

			for (size_t i = 0; i < v.size(); i++) {
				lua_pushnumber(L, v[i]->id);
				lua_rawseti(L, -2, i + 1);
			}
		}
		lua_rawset(L, -3);
	}

	return 1;
}


int LuaUnsyncedRead::GetGroupUnitsCounts(lua_State* L)
{
	const int groupID = luaL_checkint(L, 1);

	if (!uiGroupHandlers[gu->myTeam].HasGroup(groupID))
		return 0; // nils

	const CGroup* group = uiGroupHandlers[gu->myTeam].GetGroup(groupID);

	ggucCountMap.clear();
	ggucCountMap.resize(unitDefHandler->NumUnitDefs() + 1, {0, 0});

	for (const int unitID: group->units) {
		const CUnit* unit = unitHandler.GetUnit(unitID);
		const UnitDef* unitDef = unit->unitDef;

		ggucCountMap[unitDef->id].first = unitDef->id;
		ggucCountMap[unitDef->id].second += 1;
	}

	lua_createtable(L, 0, ggucCountMap.size());

	for (const auto& el: ggucCountMap) {
		if (el.second == 0)
			continue;

		lua_pushnumber(L, el.second); // push the UnitDef unit count
		lua_rawseti(L, -2, el.first); // push the UnitDef index
	}

	return 1;
}


int LuaUnsyncedRead::GetGroupUnitsCount(lua_State* L)
{
	const CGroup* group = uiGroupHandlers[gu->myTeam].GetGroup(luaL_checkint(L, 1));

	lua_pushnumber(L, group->units.size());
	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetPlayerRoster(lua_State* L)
{
	const PlayerRoster::SortType oldSortType = playerRoster.GetSortType();

	if (!lua_isnone(L, 1))
		playerRoster.SetSortTypeByCode((PlayerRoster::SortType) luaL_checkint(L, 1));

	const bool includePathingFlag = luaL_optboolean(L, 2, false);

	// get the sorted indices of *all* (including inactive) players
	const std::vector<int>& playerIndices = playerRoster.GetIndices(includePathingFlag);

	// revert
	playerRoster.SetSortTypeByCode(oldSortType);

	// push the active players
	lua_createtable(L, playerIndices.size(), 0);
	for (size_t i = 0, j = 1, s = playerIndices.size(); i < s; i++) {
		if (!AddPlayerToRoster(L, playerIndices[i], true, includePathingFlag))
			continue;

		// t[j] = {...}
		lua_rawseti(L, -2, j++);
	}

	return 1;
}

int LuaUnsyncedRead::GetPlayerTraffic(lua_State* L)
{
	const int playerID = luaL_checkint(L, 1);
	const int packetID = (int)luaL_optnumber(L, 2, -1);

	const auto& traffic = game->GetPlayerTraffic();
	const auto it = traffic.find(playerID);

	if (it == traffic.end()) {
		lua_pushnumber(L, -1);
		return 1;
	}

	// only allow viewing stats for specific packet types
	if (
		(playerID != -1) &&              // all system counts can be read
		(playerID != gu->myPlayerNum) && // all  self  counts can be read
		(packetID != -1) &&
		(packetID != NETMSG_CHAT)     &&
		(packetID != NETMSG_PAUSE)    &&
		(packetID != NETMSG_LUAMSG)   &&
		(packetID != NETMSG_STARTPOS) &&
		(packetID != NETMSG_USER_SPEED)
	) {
		lua_pushnumber(L, -1);
		return 1;
	}

	const CGame::PlayerTrafficInfo& pti = it->second;
	if (packetID == -1) {
		lua_pushnumber(L, pti.total);
		return 1;
	}

	const auto pit = pti.packets.find(packetID);
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
	if (!playerHandler.IsValidPlayer(playerID))
		return 0;

	const CPlayer* player = playerHandler.Player(playerID);
	if (player == nullptr)
		return 0;

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
	for (ConfigVariable::MetaDataMap::const_iterator it = cfgmap.begin(); it != cfgmap.end(); ++it) {
		const ConfigVariableMetaData* meta = it->second;

		lua_createtable(L, 0, 9);

			lua_pushliteral(L, "name");
			lua_pushsstring(L, meta->GetKey());
			lua_rawset(L, -3);
			if (meta->GetDescription().IsSet()) {
				lua_pushliteral(L, "description");
				lua_pushsstring(L, meta->GetDescription().ToString());
				lua_rawset(L, -3);
			}
			lua_pushliteral(L, "type");
			lua_pushsstring(L, meta->GetType());
			lua_rawset(L, -3);
			if (meta->GetDefaultValue().IsSet()) {
				lua_pushliteral(L, "defaultValue");
				lua_pushsstring(L, meta->GetDefaultValue().ToString());
				lua_rawset(L, -3);
			}
			if (meta->GetMinimumValue().IsSet()) {
				lua_pushliteral(L, "minimumValue");
				lua_pushsstring(L, meta->GetMinimumValue().ToString());
				lua_rawset(L, -3);
			}
			if (meta->GetMaximumValue().IsSet()) {
				lua_pushliteral(L, "maximumValue");
				lua_pushsstring(L, meta->GetMaximumValue().ToString());
				lua_rawset(L, -3);
			}
			if (meta->GetSafemodeValue().IsSet()) {
				lua_pushliteral(L, "safemodeValue");
				lua_pushsstring(L, meta->GetSafemodeValue().ToString());
				lua_rawset(L, -3);
			}
			if (meta->GetDeclarationFile().IsSet()) {
				lua_pushliteral(L, "declarationFile");
				lua_pushsstring(L, meta->GetDeclarationFile().ToString());
				lua_rawset(L, -3);
			}
			if (meta->GetDeclarationLine().IsSet()) {
				lua_pushliteral(L, "declarationLine");
				lua_pushnumber(L, meta->GetDeclarationLine().Get());
				lua_rawset(L, -3);
			}
			if (meta->GetReadOnly().IsSet()) {
				lua_pushliteral(L, "readOnly");
				lua_pushboolean(L, !!meta->GetReadOnly().Get());
				lua_rawset(L, -3);
			}

		lua_rawseti(L, -2, i++);
	}
	return 1;
}

int LuaUnsyncedRead::GetConfigInt(lua_State* L)
{
	lua_pushinteger(L, configHandler->GetIntSafe(luaL_checkstring(L, 1), luaL_optint(L, 2, 0)));
	return 1;
}

int LuaUnsyncedRead::GetConfigFloat(lua_State* L)
{
	lua_pushnumber(L, configHandler->GetFloatSafe(luaL_checkstring(L, 1), luaL_optfloat(L, 2, 0.0f)));
	return 1;
}

int LuaUnsyncedRead::GetConfigString(lua_State* L)
{
	lua_pushsstring(L, configHandler->GetStringSafe(luaL_checkstring(L, 1), luaL_optstring(L, 2, "")));
	return 1;
}

/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetLogSections(lua_State* L) {
	const int numLogSections = log_filter_section_getNumRegisteredSections();

	lua_createtable(L, 0, numLogSections);
	for (int i = 0; i < numLogSections; ++i) {
		const char* sectionName = log_filter_section_getRegisteredIndex(i);
		const int logLevel = log_filter_section_getMinLevel(sectionName);

		lua_pushstring(L, sectionName);
		lua_pushnumber(L, logLevel);
		lua_rawset(L, -3);
	}

	return 1;
}

/******************************************************************************/
/******************************************************************************/

int LuaUnsyncedRead::GetAllDecals(lua_State* L)
{
	const auto decalsGl4 = dynamic_cast<const CDecalsDrawerGL4*>(groundDecals);
	if (decalsGl4 == nullptr)
		return 0;

	const auto& decals = decalsGl4->GetAllDecals();

	int i = 1;
	lua_createtable(L, decals.size(), 0);
	for (const auto& d: decals) {
		if (!d.IsValid())
			continue;

		lua_pushnumber(L, d.GetIdx());
		lua_rawseti(L, -2, i++);
	}

	return 1;
}


int LuaUnsyncedRead::GetDecalPos(lua_State* L)
{
	const auto decalsGl4 = dynamic_cast<const CDecalsDrawerGL4*>(groundDecals);
	if (decalsGl4 == nullptr)
		return 0;

	const auto decal = decalsGl4->GetDecalByIdx(luaL_checkint(L, 1));
	lua_pushnumber(L, decal.pos.x);
	lua_pushnumber(L, decal.pos.y);
	lua_pushnumber(L, decal.pos.z);
	return 3;
}


int LuaUnsyncedRead::GetDecalSize(lua_State* L)
{
	const auto decalsGl4 = dynamic_cast<const CDecalsDrawerGL4*>(groundDecals);
	if (decalsGl4 == nullptr)
		return 0;

	const auto decal = decalsGl4->GetDecalByIdx(luaL_checkint(L, 1));
	lua_pushnumber(L, decal.size.x);
	lua_pushnumber(L, decal.size.y);
	return 2;
}


int LuaUnsyncedRead::GetDecalRotation(lua_State* L)
{
	const auto decalsGl4 = dynamic_cast<const CDecalsDrawerGL4*>(groundDecals);
	if (decalsGl4 == nullptr)
		return 0;

	const auto decal = decalsGl4->GetDecalByIdx(luaL_checkint(L, 1));
	lua_pushnumber(L, decal.rot);
	return 1;
}


int LuaUnsyncedRead::GetDecalTexture(lua_State* L)
{
	const auto decalsGl4 = dynamic_cast<const CDecalsDrawerGL4*>(groundDecals);
	if (decalsGl4 == nullptr)
		return 0;

	const auto decal = decalsGl4->GetDecalByIdx(luaL_checkint(L, 1));
	lua_pushsstring(L, decal.GetTexture());
	return 1;
}


int LuaUnsyncedRead::GetDecalAlpha(lua_State* L)
{
	const auto decalsGl4 = dynamic_cast<const CDecalsDrawerGL4*>(groundDecals);
	if (decalsGl4 == nullptr)
		return 0;

	const auto decal = decalsGl4->GetDecalByIdx(luaL_checkint(L, 1));
	lua_pushnumber(L, decal.alpha);
	return 1;
}


int LuaUnsyncedRead::GetDecalOwner(lua_State* L)
{
	const auto decalsGl4 = dynamic_cast<const CDecalsDrawerGL4*>(groundDecals);
	if (decalsGl4 == nullptr)
		return 0;

	const auto decal = decalsGl4->GetDecalByIdx(luaL_checkint(L, 1));

	if (decal.owner == nullptr)
		return 0;

	//XXX: I know, not very fast, but you cannot dynamic_cast a void* back to a CUnit*
	//     also it's not called very often and so doesn't matter
	for (const CUnit* u: unitHandler.GetActiveUnits()) {
		if (u != decal.owner)
			continue;

		lua_pushnumber(L, u->id);
		return 1;
	}

	return 0;
}


int LuaUnsyncedRead::GetDecalType(lua_State* L)
{
	const auto decalsGl4 = dynamic_cast<const CDecalsDrawerGL4*>(groundDecals);
	if (decalsGl4 == nullptr)
		return 0;

	const auto decal = decalsGl4->GetDecalByIdx(luaL_checkint(L, 1));
	switch (decal.type) {
		case CDecalsDrawerGL4::Decal::EXPLOSION: {
			lua_pushliteral(L, "explosion");
		} break;
		case CDecalsDrawerGL4::Decal::BUILDING: {
			lua_pushliteral(L, "building");
		} break;
		case CDecalsDrawerGL4::Decal::LUA: {
			lua_pushliteral(L, "lua");
		} break;
		default: {
			lua_pushliteral(L, "unknown");
		}
	}
	return 1;
}

