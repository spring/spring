#ifndef LUA_UI_H
#define LUA_UI_H
// LuaUI.h: interface for the CLuaUI class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include <string>
#include <vector>
#include <map>
#include "SDL_types.h"
using namespace std;

#include "Lua/LuaHandle.h"
#include "Game/command.h"


class CUnit;
struct Command;
struct lua_State;
struct CommandDescription;


class CLuaUI : public CLuaHandle {
	public:
		static void LoadHandler();
		static void FreeHandler();

		static void UpdateTeams();

		static void Reload();

	public: // structs
		struct ReStringPair {
			int cmdIndex;
			string texture;
		};

		struct ReParamsPair {
			int cmdIndex;
			map<int, string> params;
		};

	public: // call-ins
		bool HasCallIn(const string& name);
		bool UnsyncedUpdateCallIn(const string& name);

		void Shutdown();

		bool HasLayoutButtons();

		bool LayoutButtons(int& xButtons, int& yButtons,
		                   const vector<CommandDescription>& cmds,
		                   vector<int>& removeCmds,
		                   vector<CommandDescription>& customCmds,
		                   vector<int>& onlyTextureCmds,
		                   vector<ReStringPair>& reTextureCmds,
		                   vector<ReStringPair>& reNamedCmds,
		                   vector<ReStringPair>& reTooltipCmds,
		                   vector<ReParamsPair>& reParamsCmds,
		                   map<int, int>& iconList,
		                   string& menuName);

		bool ConfigCommand(const string& command);

		bool CommandNotify(const Command& cmd);

		bool KeyPress(unsigned short key, bool isRepeat);
		bool KeyRelease(unsigned short key);
		bool MouseMove(int x, int y, int dx, int dy, int button);
		bool MousePress(int x, int y, int button);
		int  MouseRelease(int x, int y, int button); // return a cmd index, or -1
		bool MouseWheel(bool up, float value);
		bool IsAbove(int x, int y);
		string GetTooltip(int x, int y);

		bool AddConsoleLines();

		bool GroupChanged(int groupID);

		void ShockFront(float power, const float3& pos, float areaOfEffect);

	public: // custom call-in
		bool HasUnsyncedXCall(const string& funcName);
		int UnsyncedXCall(lua_State* srcState, const string& funcName);

	protected:
		CLuaUI();
		~CLuaUI();

		void KillLua();

		string LoadFile(const string& filename) const;

		bool LoadCFunctions(lua_State* L);

		bool BuildCmdDescTable(lua_State* L,
		                       const vector<CommandDescription>& cmds);

		bool GetLuaIntMap(lua_State* L, int index, map<int, int>& intList);

		bool GetLuaIntList(lua_State* L, int index, vector<int>& intList);

		bool GetLuaReStringList(lua_State* L, int index,
		                        vector<ReStringPair>& reStringCmds);

		bool GetLuaReParamsList(lua_State* L, int index,
		                        vector<ReParamsPair>& reParamsCmds);

		bool GetLuaCmdDescList(lua_State* L, int index,
		                       vector<CommandDescription>& customCmds);

	protected:
		bool haveShockFront;
		float shockFrontMinArea;
		float shockFrontMinPower;
		float shockFrontDistAdj;

	private: // call-outs
		static int GetConfigInt(lua_State* L);
		static int SetConfigInt(lua_State* L);
		static int GetConfigString(lua_State* L);
		static int SetConfigString(lua_State* L);

		static int CreateDir(lua_State* L);
		static int MakeFont(lua_State* L);
		static int SetUnitDefIcon(lua_State* L);

		static int GetFPS(lua_State* L);

		static int GetMouseState(lua_State* L);
		static int WarpMouse(lua_State* L);

		static int SetCameraOffset(lua_State* L);

		static int SetShockFrontFactors(lua_State* L);

		static int SetMouseCursor(lua_State* L);
		static int GetMouseCursor(lua_State* L);

		static int GetKeyState(lua_State* L);
		static int GetModKeyState(lua_State* L);
		static int GetPressedKeys(lua_State* L);

		static int SetActiveCommand(lua_State* L);
		static int GetActiveCommand(lua_State* L);
		static int GetDefaultCommand(lua_State* L);
		static int GetActiveCmdDescs(lua_State* L);
		static int GetActiveCmdDesc(lua_State* L);
		static int GetCmdDescIndex(lua_State* L);

		static int GetActivePage(lua_State* L);
		static int ForceLayoutUpdate(lua_State* L);

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

		static int SendCommands(lua_State* L);

		static int SetShareLevel(lua_State* L);
		static int ShareResources(lua_State* L);

		static int GetMyAllyTeamID(lua_State* L);
		static int GetMyTeamID(lua_State* L);
		static int GetMyPlayerID(lua_State* L);

		static int SetUnitGroup(lua_State* L);
		static int GetUnitGroup(lua_State* L);

		static int GetGroupUnits(lua_State* L);
		static int GetGroupUnitsSorted(lua_State* L);
		static int GetGroupUnitsCounts(lua_State* L);
		static int GetGroupUnitsCount(lua_State* L);

		static int GiveOrder(lua_State* L);
		static int GiveOrderToUnit(lua_State* L);
		static int GiveOrderToUnitMap(lua_State* L);
		static int GiveOrderToUnitArray(lua_State* L);
		static int GiveOrderArrayToUnitMap(lua_State* L);
		static int GiveOrderArrayToUnitArray(lua_State* L);

		static int MarkerAddPoint(lua_State* L);
		static int MarkerAddLine(lua_State* L);
		static int MarkerErasePosition(lua_State* L);
};


extern CLuaUI* luaUI;


#endif /* LUA_UI_H */
