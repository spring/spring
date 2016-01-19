/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_RULES_H
#define LUA_RULES_H

#include <string>
using std::string;
#include <vector>
using std::vector;
#include <map>
using std::map;

#include "LuaHandleSynced.h"

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


class CLuaRules : public CLuaHandleSynced
{
	public:
		static bool ReloadHandler() { return (FreeHandler(), LoadFreeHandler()); } // NOTE the ','
		static bool LoadFreeHandler() { return (LoadHandler() || FreeHandler()); }

		static bool LoadHandler();
		static bool FreeHandler();

	public: // call-ins
		void Cob2Lua(const LuaHashString& funcName, const CUnit* unit,
		             int& argsCount, int args[MAX_LUA_COB_ARGS]);

		const char* RecvSkirmishAIMessage(int aiID, const char* data, int inSize) {
			return syncedLuaHandle.RecvSkirmishAIMessage(aiID, data, inSize);
		}

	private:
		CLuaRules();
		virtual ~CLuaRules();

	protected:
		bool AddSyncedCode(lua_State* L);
		bool AddUnsyncedCode(lua_State* L);

		int UnpackCobArg(lua_State* L);

	protected: // call-outs
		static int PermitHelperAIs(lua_State* L);

	private:
		static const int* currentCobArgs;
};


extern CLuaRules* luaRules;


#endif /* LUA_RULES_H */
