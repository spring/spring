#ifndef ICON_LAYOUT_HANDLER_H
#define ICON_LAYOUT_HANDLER_H
// IconLayoutHandler.h: interface for the CIconLayoutHandler class.
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


class CIconLayoutHandler {
	public:
		static CIconLayoutHandler* GetHandler(const string& filename);
		
		~CIconLayoutHandler();

		struct IconData {
			enum { Command, Action, Dead } type;
			int cmdIndex;
			string action;
			string label;
			string cursor;
			string texture;
			string tooltip;
		};
		typedef vector<IconData> IconDataList;

		bool ConfigCommand(const string& command);
		
		bool UpdateLayout(bool& forceLayout, bool commandsChanged, int activePage);
		
		struct ReStringPair {
			int cmdIndex;
			string texture;
		};

		struct ReParamsPair {
			int cmdIndex;
			map<int, string> params;
		};

		bool LayoutIcons(int& xIcons, int& yIcons,
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

		bool CommandNotify(const Command& cmd);
		
		bool DrawMapItems();
		bool DrawScreenItems();

		bool MouseMove(int x, int y, int dx, int dy, int button);
		bool MousePress(int x, int y, int button);
		bool MouseRelease(int x, int y, int button);
		
		bool UnitCreated(CUnit* unit);
		bool UnitReady(CUnit* unit, CUnit* builder);
		bool UnitDestroyed(CUnit* victim, CUnit* attacker);
		
	private:
		CIconLayoutHandler();

		string LoadFile(const string& filename);
		
		bool LoadCFunctions(lua_State* L);
		
		bool LoadCode(lua_State* L, const string& code, const string& debug);
		
		bool BuildCmdDescTable(lua_State* L,
		                       const vector<CommandDescription>& cmds);

		bool GetLuaIntMap(lua_State* L, map<int, int>& intList);

		bool GetLuaIntList(lua_State* L, vector<int>& intList);

		bool GetLuaReStringList(lua_State* L,
		                        vector<ReStringPair>& reStringCmds);

		bool GetLuaReParamsList(lua_State* L,
		                        vector<ReParamsPair>& reParamsCmds);

		bool GetLuaCmdDescList(lua_State* L,
		                       vector<CommandDescription>& customCmds);
};


#endif /* ICON_LAYOUT_HANDLER_H */
