#ifndef LUA_UI_H
#define LUA_UI_H
// LuaUI.h: interface for the CLuaUI class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <string>
#include <vector>
#include <map>
using namespace std;

#include "Game/command.h"

extern "C" {
	#include "lua.h"
}


class CUnit;
struct Command;
struct lua_State;
struct CommandDescription;


class CLuaUI {
	public:
		static CLuaUI* GetHandler(const string& filename);
		
		~CLuaUI();
		
		void Shutdown();

		struct ReStringPair {
			int cmdIndex;
			string texture;
		};

		struct ReParamsPair {
			int cmdIndex;
			map<int, string> params;
		};

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
		
		bool UpdateLayout(bool commandsChanged, int activePage);
		
		bool CommandNotify(const Command& cmd);
		
		bool DrawWorldItems();
		bool DrawScreenItems();

		bool KeyPress(unsigned short key, bool isRepeat);
		bool KeyRelease(unsigned short key);
		bool MouseMove(int x, int y, int dx, int dy, int button);
		bool MousePress(int x, int y, int button);
		int  MouseRelease(int x, int y, int button); // return a cmd index, or -1
		bool IsAbove(int x, int y);
		string GetTooltip(int x, int y);
		
		bool AddConsoleLine(const string& line, int priority);
		
		bool UnitCreated(CUnit* unit);
		bool UnitFinished(CUnit* unit);
		bool UnitReady(CUnit* unit, CUnit* builder);
		bool UnitDestroyed(CUnit* victim, CUnit* attacker);
		bool UnitChangedTeam(CUnit* unit, int oldTeam, int newTeam);
		
		bool GroupChanged(int groupID);
		
	private:
		CLuaUI();

		string LoadFile(const string& filename);
		
		bool LoadCFunctions(lua_State* L);
		
		bool LoadCode(lua_State* L, const string& code, const string& debug);
		
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
};


#endif /* LUA_UI_H */
