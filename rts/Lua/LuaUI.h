/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_UI_H
#define LUA_UI_H

#include <string>
#include <vector>
#include <map>

#include "LuaHandle.h"


class CUnit;
class CFeature;
struct Command;
struct lua_State;
struct CommandDescription;
class LuaLobby;


class CLuaUI : public CLuaHandle
{
	friend class LuaLobby;

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
		bool HasCallIn(lua_State *L, const string& name);
		bool UnsyncedUpdateCallIn(lua_State *L, const string& name);

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

		void ShockFront(float power, const float3& pos, float areaOfEffect, float *distadj = NULL);

	public: // custom call-in
		bool HasUnsyncedXCall(lua_State* srcState, const string& funcName);
		int UnsyncedXCall(lua_State* srcState, const string& funcName);
		void ExecuteUIEventBatch();

	protected:
		CLuaUI();
		~CLuaUI();

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
		static int SetShockFrontFactors(lua_State* L);

		int UpdateUnsyncedXCalls(lua_State* L);
		/**
		*	initialize luasocket
		*/
		void InitLuaSocket(lua_State* L);
		std::set<std::string> unsyncedXCalls;
		std::vector<LuaUIEvent> luaUIEventBatch;
};


extern CLuaUI* luaUI;


#endif /* LUA_UI_H */
