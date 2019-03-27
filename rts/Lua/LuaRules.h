/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_RULES_H
#define LUA_RULES_H

#include <string>
#include <vector>

#include "LuaHandleSynced.h"
#include "System/UnorderedMap.hpp"

#define MAX_LUA_COB_ARGS 10


class CUnit;
class CFeature;
class CProjectile;
class CWeapon;
struct UnitDef;
struct FeatureDef;
struct Command;
struct BuildInfo;
struct lua_State;


class CLuaRules : public CSplitLuaHandle
{
public:
	static bool CanLoadHandler() { return true; }
	static bool ReloadHandler(bool onlySynced = false) { return (FreeHandler(), LoadFreeHandler(onlySynced)); } // NOTE the ','
	static bool LoadFreeHandler(bool onlySynced = false) { return (LoadHandler(onlySynced) || FreeHandler()); }

	static bool LoadHandler(bool onlySynced);
	static bool FreeHandler();

public: // call-ins
	void Cob2Lua(const LuaHashString& funcName, const CUnit* unit,
	             int& argsCount, int args[MAX_LUA_COB_ARGS]);

	const char* RecvSkirmishAIMessage(int aiID, const char* data, int inSize, size_t* outSize) {
		return syncedLuaHandle.RecvSkirmishAIMessage(aiID, data, inSize, outSize);
	}


private:
	CLuaRules(bool onlySynced);
	virtual ~CLuaRules();

protected:
	bool AddSyncedCode(lua_State* L);
	bool AddUnsyncedCode(lua_State* L);

	std::string GetUnsyncedFileName() const;
	std::string GetSyncedFileName() const;
	std::string GetInitFileModes() const;
	int GetInitSelectTeam() const;

	int UnpackCobArg(lua_State* L);

protected: // call-outs
	static int PermitHelperAIs(lua_State* L);

private:
	static const int* currentCobArgs;
};


extern CLuaRules* luaRules;


#endif /* LUA_RULES_H */
