#ifndef LUA_UNSYNCED_CTRL_H
#define LUA_UNSYNCED_CTRL_H
// LuaUnsyncedCtrl.h: interface for the LuaUnsyncedCtrl class.
//
//////////////////////////////////////////////////////////////////////

#include "Sim/Units/UnitSet.h"

struct lua_State;


class LuaUnsyncedCtrl {
	public:
		static bool PushEntries(lua_State* L);

		static void DrawUnitCommandQueues();
		static void ClearUnitCommandQueues();

	private:
		static CUnitSet drawCmdQueueUnits;

	private:
		static int Echo(lua_State* L);

		static int SendMessage(lua_State* L);
		static int SendMessageToPlayer(lua_State* L);
		static int SendMessageToTeam(lua_State* L);
		static int SendMessageToAllyTeam(lua_State* L);
		static int SendMessageToSpectators(lua_State* L);

		static int PlaySoundFile(lua_State* L);
		static int PlaySoundStream(lua_State* L);
		static int StopSoundStream(lua_State* L);
		static int PauseSoundStream(lua_State* L);
		static int GetSoundStreamTime(lua_State* L);
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

		static int SetUnitNoDraw(lua_State* L);
		static int SetUnitNoMinimap(lua_State* L);
		static int SetUnitNoSelect(lua_State* L);

		static int AddUnitIcon(lua_State* L);
		static int FreeUnitIcon(lua_State* L);

		static int ExtractModArchiveFile(lua_State* L);
};


#endif /* LUA_UNSYNCED_CTRL_H */
