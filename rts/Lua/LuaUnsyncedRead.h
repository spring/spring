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

		static int GetFrameTimeOffset(lua_State* L);
		static int GetLastUpdateSeconds(lua_State* L);
		static int GetHasLag(lua_State* L);

		static int IsAABBInView(lua_State* L);
		static int IsSphereInView(lua_State* L);

		static int IsUnitAllied(lua_State* L);
		static int IsUnitInView(lua_State* L);
		static int IsUnitVisible(lua_State* L);
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
};


#endif /* LUA_UNSYNCED_INFO_H */
