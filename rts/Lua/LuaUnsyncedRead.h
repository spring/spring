#ifndef LUA_UNSYNCED_INFO_H
#define LUA_UNSYNCED_INFO_H
// LuaUnsyncedRead.h: interface for the LuaUnsyncedRead class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif


struct lua_State;


class LuaUnsyncedRead {
	public:
		static bool PushEntries(lua_State* L);

		static void AllowGameChanges(bool value);

	private:
		static int IsReplay(lua_State* L);
		static int IsGUIHidden(lua_State* L);

		static int GetFrameTimeOffset(lua_State* L);
		static int IsSphereInView(lua_State* L);
		static int IsUnitInView(lua_State* L);
		static int IsUnitSelected(lua_State* L);
		static int GetUnitViewPosition(lua_State* L);

		static int GetPlayerRoster(lua_State* L);

		static int GetLocalPlayerID(lua_State* L);
		static int GetLocalTeamID(lua_State* L);
		static int GetLocalAllyTeamID(lua_State* L);
		static int GetSpectatingState(lua_State* L);

		static int GetSelectedUnits(lua_State* L);
		static int GetSelectedUnitsSorted(lua_State* L);
		static int GetSelectedUnitsCounts(lua_State* L);

		static int GetCameraNames(lua_State* L);
		static int GetCameraState(lua_State* L);
		static int GetCameraPosition(lua_State* L);
		static int GetCameraVectors(lua_State* L);
		static int WorldToScreenCoords(lua_State* L);
		static int TraceScreenRay(lua_State* L);

		static int GetTimer(lua_State* L);
		static int DiffTimers(lua_State* L);
};


#endif /* LUA_UNSYNCED_INFO_H */
