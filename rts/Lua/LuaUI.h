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
		bool HasCallIn(lua_State* L, const string& name);

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

		void ShockFront(const float3& pos, float power, float areaOfEffect, const float* distMod = NULL);

	protected:
		CLuaUI();
		virtual ~CLuaUI();

		string LoadFile(const string& name, const std::string& mode) const;

		bool LoadCFunctions(lua_State* L);
		void InitLuaSocket(lua_State* L);

		bool BuildCmdDescTable(lua_State* L, const vector<CommandDescription>& cmds);
		bool GetLuaIntMap(lua_State* L, int index, map<int, int>& intList);
		bool GetLuaIntList(lua_State* L, int index, vector<int>& intList);
		bool GetLuaReStringList(lua_State* L, int index, vector<ReStringPair>& reStringCmds);
		bool GetLuaReParamsList(lua_State* L, int index, vector<ReParamsPair>& reParamsCmds);
		bool GetLuaCmdDescList(lua_State* L, int index,  vector<CommandDescription>& customCmds);

	protected:
		bool haveShockFront;
		float shockFrontMinArea;
		float shockFrontMinPower;
		float shockFrontDistAdj;

	private: // call-outs
		static int SetShockFrontFactors(lua_State* L);
};


extern CLuaUI* luaUI;


#endif /* LUA_UI_H */
