/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_UNSYNCED_CTRL_H
#define LUA_UNSYNCED_CTRL_H

struct lua_State;

// MinGW defines this for a WINAPI function
#undef SendMessage

class LuaUnsyncedCtrl {
	friend class CLuaIntro;

	public:
		static bool PushEntries(lua_State* L);

	public:
		static int Ping(lua_State* L);
		static int Echo(lua_State* L);
		static int Log(lua_State* L);
		static int SendMessage(lua_State* L);
		static int SendMessageToPlayer(lua_State* L);
		static int SendMessageToTeam(lua_State* L);
		static int SendMessageToAllyTeam(lua_State* L);
		static int SendMessageToSpectators(lua_State* L);

		static int LoadSoundDef(lua_State* L);
		static int PlaySoundFile(lua_State* L);
		static int PlaySoundStream(lua_State* L);
		static int StopSoundStream(lua_State* L);
		static int PauseSoundStream(lua_State* L);
		static int SetSoundStreamVolume(lua_State* L);

		static int SetCameraState(lua_State* L);
		static int SetCameraTarget(lua_State* L);

		static int SelectUnitMap(lua_State* L);
		static int SelectUnitArray(lua_State* L);

		static int AddWorldIcon(lua_State* L);
		static int AddWorldText(lua_State* L);
		static int AddWorldUnit(lua_State* L);

		static int DrawUnitCommands(lua_State* L);

		static int SetTeamColor(lua_State* L);

		static int AssignMouseCursor(lua_State* L);
		static int ReplaceMouseCursor(lua_State* L);

		static int SetCustomCommandDrawData(lua_State* L);

		static int SetDrawSky(lua_State* L);
		static int SetDrawWater(lua_State* L);
		static int SetDrawGround(lua_State* L);
		static int SetDrawGroundDeferred(lua_State* L);
		static int SetDrawModelsDeferred(lua_State* L);
		static int SetVideoCapturingMode(lua_State* L);
		static int SetVideoCapturingTimeOffset(lua_State* L);

		static int SetWaterParams(lua_State* L);
		static int SetSoundEffectParams(lua_State* L);

		static int AddMapLight(lua_State* L);
		static int AddModelLight(lua_State* L);
		static int UpdateMapLight(lua_State* L);
		static int UpdateModelLight(lua_State* L);
		static int SetMapLightTrackingState(lua_State* L);
		static int SetModelLightTrackingState(lua_State* L);
		static int SetMapShader(lua_State* L);
		static int SetMapSquareTexture(lua_State* L);
		static int SetMapShadingTexture(lua_State* L);
		static int SetSkyBoxTexture(lua_State* L);

		static int SetUnitNoDraw(lua_State* L);
		static int SetUnitNoMinimap(lua_State* L);
		static int SetUnitNoSelect(lua_State* L);
		static int SetUnitLeaveTracks(lua_State* L);
		static int SetUnitSelectionVolumeData(lua_State* L);
		static int SetFeatureNoDraw(lua_State* L);
		static int SetFeatureFade(lua_State* L);
		static int SetFeatureSelectionVolumeData(lua_State* L);

		static int AddUnitIcon(lua_State* L);
		static int FreeUnitIcon(lua_State* L);

		static int ExtractModArchiveFile(lua_State* L);


		// moved from LuaUI

//FIXME		static int SetShockFrontFactors(lua_State* L);

		static int SetConfigInt(lua_State* L);
		static int SetConfigFloat(lua_State* L);
		static int SetConfigString(lua_State* L);

		static int CreateDir(lua_State* L);

		static int Reload(lua_State* L);
		static int Restart(lua_State* L);
		static int Start(lua_State* L);
		static int Quit(lua_State* L);

		static int SetWMIcon(lua_State* L);
		static int SetWMCaption(lua_State* L);

		static int SetUnitDefIcon(lua_State* L);
		static int SetUnitDefImage(lua_State* L);

		static int SetActiveCommand(lua_State* L);

		static int LoadCmdColorsConfig(lua_State* L);
		static int LoadCtrlPanelConfig(lua_State* L);
		static int ForceLayoutUpdate(lua_State* L);

		static int SetLosViewColors(lua_State* L);

		static int WarpMouse(lua_State* L);

		static int SetMouseCursor(lua_State* L);
		static int SetClipboard(lua_State* L);

		static int SetCameraOffset(lua_State* L);

		static int SendCommands(lua_State* L);

		static int SetShareLevel(lua_State* L);
		static int ShareResources(lua_State* L);

		static int SetUnitGroup(lua_State* L);

		static int GiveOrder(lua_State* L);
		static int GiveOrderToUnit(lua_State* L);
		static int GiveOrderToUnitMap(lua_State* L);
		static int GiveOrderToUnitArray(lua_State* L);
		static int GiveOrderArrayToUnitMap(lua_State* L);
		static int GiveOrderArrayToUnitArray(lua_State* L);

		static int SendLuaUIMsg(lua_State* L);
		static int SendLuaGaiaMsg(lua_State* L);
		static int SendLuaRulesMsg(lua_State* L);
		static int SendLuaMenuMsg(lua_State* L);

		static int SetLastMessagePosition(lua_State* L);

		static int MarkerAddPoint(lua_State* L);
		static int MarkerAddLine(lua_State* L);
		static int MarkerErasePosition(lua_State* L);

		static int SetDrawSelectionInfo(lua_State* L);

		static int SetBuildSpacing(lua_State* L);
		static int SetBuildFacing(lua_State* L);

		static int SetAtmosphere(lua_State* L);
		static int SetSunLighting(lua_State* L);
		static int SetSunDirection(lua_State* L);
		static int SetMapRenderingParams(lua_State* L);

		static int SendSkirmishAIMessage(lua_State* L);

		static int SetLogSectionFilterLevel(lua_State* L);

		static int ClearWatchDogTimer(lua_State* L);
		static int GarbageCollectCtrl(lua_State* L);

		static int PreloadUnitDefModel(lua_State* L);
		static int PreloadFeatureDefModel(lua_State* L);
		static int PreloadSoundItem(lua_State* L);

		static int CreateDecal(lua_State* L);
		static int DestroyDecal(lua_State* L);
		static int SetDecalPos(lua_State* L);
		static int SetDecalSize(lua_State* L);
		static int SetDecalRotation(lua_State* L);
		static int SetDecalTexture(lua_State* L);
		static int SetDecalAlpha(lua_State* L);

		static int SDLSetTextInputRect(lua_State* L);
		static int SDLStartTextInput(lua_State* L);
		static int SDLStopTextInput(lua_State* L);
};

#endif /* LUA_UNSYNCED_CTRL_H */
