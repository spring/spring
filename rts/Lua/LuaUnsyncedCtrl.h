#ifndef LUA_UNSYNCED_CTRL_H
#define LUA_UNSYNCED_CTRL_H
// LuaUnsyncedCtrl.h: interface for the LuaUnsyncedCtrl class.
//
//////////////////////////////////////////////////////////////////////

#include "Sim/Units/UnitSet.h"

struct lua_State;

// MinGW defines this for a WINAPI function
#undef SendMessage

class LuaUnsyncedCtrl {
	public:
		static bool PushEntries(lua_State* L);

		static void DrawUnitCommandQueues();
		static void ClearUnitCommandQueues();
		static CUnitSet drawCmdQueueUnits;

	private:

	private:
		static int Echo(lua_State* L);

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

		static int SetWaterParams(lua_State* L);

		static int SetUnitNoDraw(lua_State* L);
		static int SetUnitNoMinimap(lua_State* L);
		static int SetUnitNoSelect(lua_State* L);

		static int AddUnitIcon(lua_State* L);
		static int FreeUnitIcon(lua_State* L);

		static int ExtractModArchiveFile(lua_State* L);


		// moved from LuaUI

//FIXME		static int SetShockFrontFactors(lua_State* L);

		static int GetConfigInt(lua_State* L);
		static int SetConfigInt(lua_State* L);
		static int GetConfigString(lua_State* L);
		static int SetConfigString(lua_State* L);

		static int CreateDir(lua_State* L);
		static int MakeFont(lua_State* L);

		static int SetUnitDefIcon(lua_State* L);
		static int SetUnitDefImage(lua_State* L);

		static int SetActiveCommand(lua_State* L);

		static int ForceLayoutUpdate(lua_State* L);

		static int SetLosViewColors(lua_State* L);

		static int WarpMouse(lua_State* L);

		static int SetMouseCursor(lua_State* L);

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

		static int SetLastMessagePosition(lua_State* L);

		static int MarkerAddPoint(lua_State* L);
		static int MarkerAddLine(lua_State* L);
		static int MarkerErasePosition(lua_State* L);

		static int SetDrawSelectionInfo(lua_State* L);

		static int SetBuildSpacing(lua_State* L);
		static int SetBuildFacing(lua_State* L);
};


#endif /* LUA_UNSYNCED_CTRL_H */
