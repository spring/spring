#ifndef LUA_UNSYNCED_CTRL_H
#define LUA_UNSYNCED_CTRL_H
// LuaUnsyncedCtrl.h: interface for the LuaUnsyncedCtrl class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif


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

		static int PlaySoundFile(lua_State* L);

		static int SetCameraState(lua_State* L);
		static int SetCameraTarget(lua_State* L);

		static int SelectUnitMap(lua_State* L);
		static int SelectUnitArray(lua_State* L);

		static int AddWorldIcon(lua_State* L);
		static int AddWorldText(lua_State* L);
		static int AddWorldUnit(lua_State* L);

		static int DrawUnitCommands(lua_State* L);

		static int AssignMouseCursor(lua_State* L);
		static int ReplaceMouseCursor(lua_State* L);
};


#endif /* LUA_UNSYNCED_CTRL_H */
