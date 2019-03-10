/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_UI_H
#define LUA_UI_H

#include <string>
#include <vector>

#include "LuaHandle.h"
#include "System/UnorderedMap.hpp"


class CUnit;
class CFeature;
struct Command;
struct lua_State;
struct SCommandDescription;


class CLuaUI : public CLuaHandle
{
public: // structs
	enum QueuedAction {
		ACTION_RELOAD  =  0,
		ACTION_DISABLE =  1,
		ACTION_NOVALUE = -1,
	};

	struct ReStringPair {
		int cmdIndex;
		string texture;
	};

	struct ReParamsPair {
		int cmdIndex;
		spring::unordered_map<int, string> params;
	};

public:
	void QueueAction(const QueuedAction action) { queuedAction = action; }
	void CheckAction() {
		switch (queuedAction) {
			case ACTION_RELOAD:  { ReloadHandler(); } break;
			case ACTION_DISABLE: {   FreeHandler(); } break;
			default:             {                  } break;
		}
	}

	static bool CanLoadHandler() { return true; }
	static bool ReloadHandler() { return (FreeHandler(), LoadFreeHandler()); } // NOTE the ','
	static bool LoadFreeHandler() { return (LoadHandler() || FreeHandler()); }

	static bool LoadHandler();
	static bool FreeHandler();

	static void UpdateTeams();

public: // call-ins
	bool HasCallIn(lua_State* L, const string& name) const;

	bool LayoutButtons(int& xButtons, int& yButtons,
	                   const vector<SCommandDescription>& cmds,
	                   vector<int>& removeCmds,
	                   vector<SCommandDescription>& customCmds,
	                   vector<int>& onlyTextureCmds,
	                   vector<ReStringPair>& reTextureCmds,
	                   vector<ReStringPair>& reNamedCmds,
	                   vector<ReStringPair>& reTooltipCmds,
	                   vector<ReParamsPair>& reParamsCmds,
	                   spring::unordered_map<int, int>& iconList,
	                   string& menuName);

	bool ConfigureLayout(const string& command);

	void ShockFront(const float3& pos, float power, float areaOfEffect, const float* distMod = NULL);

protected:
	CLuaUI();
	virtual ~CLuaUI();

	string LoadFile(const string& name, const std::string& mode) const;

	bool LoadCFunctions(lua_State* L);
	void InitLuaSocket(lua_State* L);

	bool BuildCmdDescTable(lua_State* L, const vector<SCommandDescription>& cmds);
	bool GetLuaIntMap(lua_State* L, int index, spring::unordered_map<int, int>& intList);
	bool GetLuaIntList(lua_State* L, int index, vector<int>& intList);
	bool GetLuaReStringList(lua_State* L, int index, vector<ReStringPair>& reStringCmds);
	bool GetLuaReParamsList(lua_State* L, int index, vector<ReParamsPair>& reParamsCmds);
	bool GetLuaCmdDescList(lua_State* L, int index,  vector<SCommandDescription>& customCmds);

protected:
	QueuedAction queuedAction;

	bool haveShockFront;
	float shockFrontMinArea;
	float shockFrontMinPower;
	float shockFrontDistAdj;

private: // call-outs
	static int SetShockFrontFactors(lua_State* L);
};


extern CLuaUI* luaUI;


#endif /* LUA_UI_H */
