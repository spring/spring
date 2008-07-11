#ifndef LUA_UNSYNCED_INFO_H
#define LUA_UNSYNCED_INFO_H
// LuaUnsyncedRead.h: interface for the LuaUnsyncedRead class.
//
//////////////////////////////////////////////////////////////////////

struct lua_State;


class LuaUnsyncedRead {
	public:
		static bool PushEntries(lua_State* L);

	private:
		static int IsReplay(lua_State* L);
		static int GetModUICtrl(lua_State* L);

		static int GetDrawFrame(lua_State* L);
		static int GetFrameTimeOffset(lua_State* L);
		static int GetLastUpdateSeconds(lua_State* L);
		static int GetHasLag(lua_State* L);

		static int GetViewGeometry(lua_State* L);
		static int GetWindowGeometry(lua_State* L);
		static int GetScreenGeometry(lua_State* L);
		static int GetMiniMapGeometry(lua_State* L);
		static int GetMiniMapDualScreen(lua_State* L);

		static int IsAABBInView(lua_State* L);
		static int IsSphereInView(lua_State* L);

		static int IsUnitAllied(lua_State* L);
		static int IsUnitInView(lua_State* L);
		static int IsUnitVisible(lua_State* L);
		static int IsUnitIcon(lua_State* L);
		static int IsUnitSelected(lua_State* L);

		static int GetUnitLuaDraw(lua_State* L);
		static int GetUnitNoDraw(lua_State* L);
		static int GetUnitNoMinimap(lua_State* L);
		static int GetUnitNoSelect(lua_State* L);

		static int GetUnitViewPosition(lua_State* L);

		static int GetVisibleUnits(lua_State* L);

		static int GetPlayerRoster(lua_State* L);

		static int GetTeamColor(lua_State* L);
		static int GetTeamOrigColor(lua_State* L);

		static int GetLocalPlayerID(lua_State* L);
		static int GetLocalTeamID(lua_State* L);
		static int GetLocalAllyTeamID(lua_State* L);
		static int GetSpectatingState(lua_State* L);

		static int GetSelectedUnits(lua_State* L);
		static int GetSelectedUnitsSorted(lua_State* L);
		static int GetSelectedUnitsCounts(lua_State* L);
		static int GetSelectedUnitsCount(lua_State* L);

		static int IsGUIHidden(lua_State* L);
		static int HaveShadows(lua_State* L);
		static int HaveAdvShading(lua_State* L);
		static int GetWaterMode(lua_State* L);
		static int GetMapDrawMode(lua_State* L);

		static int GetCameraNames(lua_State* L);
		static int GetCameraState(lua_State* L);
		static int GetCameraPosition(lua_State* L);
		static int GetCameraDirection(lua_State* L);
		static int GetCameraFOV(lua_State* L);
		static int GetCameraVectors(lua_State* L);
		static int WorldToScreenCoords(lua_State* L);
		static int TraceScreenRay(lua_State* L);

		static int GetTimer(lua_State* L);
		static int DiffTimers(lua_State* L);

		// moved from LuaUI
		static int GetFPS(lua_State* L);

		static int GetMouseState(lua_State* L);
		static int GetMouseCursor(lua_State* L);
		static int GetMouseStartPosition(lua_State* L);

		static int GetKeyState(lua_State* L);
		static int GetModKeyState(lua_State* L);
		static int GetPressedKeys(lua_State* L);
		static int GetInvertQueueKey(lua_State* L);

		static int GetActiveCommand(lua_State* L);
		static int GetDefaultCommand(lua_State* L);
		static int GetActiveCmdDescs(lua_State* L);
		static int GetActiveCmdDesc(lua_State* L);
		static int GetCmdDescIndex(lua_State* L);

		static int GetGatherMode(lua_State* L);

		static int GetBuildFacing(lua_State* L);
		static int GetBuildSpacing(lua_State* L);

		static int GetActivePage(lua_State* L);

		static int GetConsoleBuffer(lua_State* L);
		static int GetCurrentTooltip(lua_State* L);

		static int GetKeyCode(lua_State* L);
		static int GetKeySymbol(lua_State* L);
		static int GetKeyBindings(lua_State* L);
		static int GetActionHotKeys(lua_State* L);

		static int GetGroupList(lua_State* L);
		static int GetSelectedGroup(lua_State* L);
		static int GetGroupAIName(lua_State* L);
		static int GetGroupAIList(lua_State* L);

		static int GetMyAllyTeamID(lua_State* L);
		static int GetMyTeamID(lua_State* L);
		static int GetMyPlayerID(lua_State* L);

		static int GetUnitGroup(lua_State* L);

		static int GetGroupUnits(lua_State* L);
		static int GetGroupUnitsSorted(lua_State* L);
		static int GetGroupUnitsCounts(lua_State* L);
		static int GetGroupUnitsCount(lua_State* L);

		static int GetPlayerTraffic(lua_State* L);
};


#endif /* LUA_UNSYNCED_INFO_H */
