/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_UTILS_H
#define LUA_UTILS_H

#include <string>
#include <vector>

#include "LuaHashString.h"
#include "LuaInclude.h"
#include "LuaHandle.h"
#include "LuaDefs.h"
// FIXME: use fwd-decls
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Features/Feature.h"
#include "Sim/Units/Unit.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/EventClient.h"

// is defined as macro on FreeBSD (wtf)
#ifdef isnumber
	#undef isnumber
#endif

struct SolidObjectDef;
struct SCommandDescription;

class LuaUtils {
	public:
		struct ScopedStackChecker {
		public:
			ScopedStackChecker(lua_State* L, int returnVars = 0);
			~ScopedStackChecker();
		private:
			lua_State* luaState;
			int prevTop;
			int returnVars;
		};

		struct ScopedDebugTraceBack {
		public:
			ScopedDebugTraceBack(lua_State* L);
			~ScopedDebugTraceBack();
			void SetErrFuncIdx(int idx) { errFuncIdx = idx; }
			int GetErrFuncIdx() const { return errFuncIdx; }

		private:
			lua_State* L;
			int errFuncIdx;
		};

	public:
		struct DataDump {
			int type;
			std::string str;
			float num;
			bool bol;
			std::vector<std::pair<DataDump, DataDump> > table;
		};
		struct ShallowDataDump {
			int type;
			union {
				std::string *str;
				float num;
				bool bol;
			} data;
		};

	public:
		// Backups lua data into a c++ vector and restores it from it
		static int exportedDataSize; //< performance stat
		static int Backup(std::vector<DataDump> &backup, lua_State* src, int count);
		static int Restore(const std::vector<DataDump> &backup, lua_State* dst);

		// Copies lua data between 2 lua_States
		static int CopyData(lua_State* dst, lua_State* src, int count);

		// returns stack index of traceback function
		static int PushDebugTraceback(lua_State* L);

		// lower case all keys in the table, with recursion
		static bool LowerKeys(lua_State* L, int tableIndex);

		static bool CheckTableForNaNs(lua_State* L, int table, const std::string& name);

		static int PushModelHeight(lua_State* L, const SolidObjectDef* def, bool isUnitDef);
		static int PushModelRadius(lua_State* L, const SolidObjectDef* def, bool isUnitDef);
		static int PushFeatureModelDrawType(lua_State* L, const FeatureDef* def);
		static int PushModelName(lua_State* L, const SolidObjectDef* def);
		static int PushModelType(lua_State* L, const SolidObjectDef* def);
		static int PushModelPath(lua_State* L, const SolidObjectDef* def);

		static int PushModelTable(lua_State* L, const SolidObjectDef* def);
		static int PushColVolTable(lua_State* L, const CollisionVolume* vol);
		static int PushColVolData(lua_State* L, const CollisionVolume* vol);
		static int ParseColVolData(lua_State* L, int idx, CollisionVolume* vol);

		static void PushCommandParamsTable(lua_State* L, const Command& cmd, bool subtable);
		static void PushCommandOptionsTable(lua_State* L, const Command& cmd, bool subtable);
		static void PushUnitAndCommand(lua_State* L, const CUnit* unit, const Command& cmd);

		static Command ParseCommand(lua_State* L, const char* caller, int idIndex);
		static Command ParseCommandTable(lua_State* L, const char* caller, int table);
		static void ParseCommandArray(lua_State* L, const char* caller, int table, vector<Command>& commands);
		static int ParseFacing(lua_State* L, const char* caller, int index);

		static void PushCurrentFuncEnv(lua_State* L, const char* caller);

		static void* GetUserData(lua_State* L, int index, const string& type);

		static void PrintStack(lua_State* L);

		static int IsEngineMinVersion(lua_State* L);

		// from LuaFeatureDefs.cpp / LuaUnitDefs.cpp / LuaWeaponDefs.cpp
		// (helper for the Next() iteration routine)
		static int Next(const ParamMap& paramMap, lua_State* L);

		// from LuaParser.cpp / LuaUnsyncedCtrl.cpp
		// (implementation copied from lua/src/lib/lbaselib.c)
		static int Echo(lua_State* L);
		static int ParseLogLevel(lua_State* L, int index);
		static int Log(lua_State* L);
		static bool PushLogEntries(lua_State* L);

		static bool PushCustomBaseFunctions(lua_State* L);

		static int ParseIntArray(lua_State* L, int tableIndex,
		                         int* array, int arraySize);
		static int ParseFloatArray(lua_State* L, int tableIndex,
		                           float* array, int arraySize);
		static int ParseStringArray(lua_State* L, int tableIndex,
		                            string* array, int arraySize);

		static int ParseIntVector(lua_State* L, int tableIndex,
		                          vector<int>& vec);
		static int ParseFloatVector(lua_State* L, int tableIndex,
		                            vector<float>& vec);
		static int ParseStringVector(lua_State* L, int tableIndex,
		                             vector<string>& vec);

		static void PushStringVector(lua_State* L, const vector<string>& vec);

		static void PushCommandDesc(lua_State* L, const SCommandDescription& cd);
};



template<typename ObjectDefType, size_t indxFuncsSize, size_t iterFuncsSize>
static void PushObjectDefProxyTable(
	lua_State* L,
	const std::array<const LuaHashString, indxFuncsSize>& indxOpers,
	const std::array<const LuaHashString, iterFuncsSize>& iterOpers,
	const std::array<const lua_CFunction, indxFuncsSize>& indxFuncs,
	const std::array<const lua_CFunction, iterFuncsSize>& iterFuncs,
	const ObjectDefType* def
) {
	lua_pushnumber(L, def->id);
	lua_newtable(L); { // the proxy table

		lua_newtable(L); // the metatable

		for (size_t n = 0; n < indxFuncsSize; n++) {
			indxOpers[n].Push(L);
			lua_pushlightuserdata(L, (void*) def);
			lua_pushcclosure(L, indxFuncs[n], 1);
			lua_rawset(L, -3); // closure
		}

		lua_setmetatable(L, -2);
	}

	for (size_t n = 0; n < iterFuncsSize; n++) {
		iterOpers[n].Push(L);
		lua_pushcfunction(L, iterFuncs[n]);
		lua_rawset(L, -3);
	}

	lua_rawset(L, -3); // set the proxy table
}



static inline void LuaPushNamedNil(lua_State* L,
                             const string& key)
{
	lua_pushsstring(L, key);
	lua_pushnil(L);
	lua_rawset(L, -3);
}


static inline void LuaPushNamedBool(lua_State* L, const string& key, bool value)
{
	lua_pushsstring(L, key);
	lua_pushboolean(L, value);
	lua_rawset(L, -3);
}


static inline void LuaPushNamedNumber(lua_State* L, const string& key, lua_Number value)
{
	lua_pushsstring(L, key);
	lua_pushnumber(L, value);
	lua_rawset(L, -3);
}


static inline void LuaPushNamedString(lua_State* L, const string& key, const string& value)
{
	lua_pushsstring(L, key);
	lua_pushsstring(L, value);
	lua_rawset(L, -3);
}


static inline void LuaPushNamedCFunc(lua_State* L, const string& key, lua_CFunction func)
{
	lua_pushsstring(L, key);
	lua_pushcfunction(L, func);
	lua_rawset(L, -3);
}

static inline void LuaPushRawNamedCFunc(lua_State* L, const char* key, lua_CFunction func)
{
	lua_pushstring(L, key);
	lua_pushcfunction(L, func);
	lua_rawset(L, -3);
}

#define REGISTER_LUA_CFUNC(func)                LuaPushRawNamedCFunc(L, #func,        func)
#define REGISTER_NAMED_LUA_CFUNC(name, func)    LuaPushRawNamedCFunc(L,  name,        func)
#define REGISTER_SCOPED_LUA_CFUNC(scope, func)  LuaPushRawNamedCFunc(L, #func, scope::func)


static inline void LuaInsertDualMapPair(lua_State* L, const string& name, int number)
{
	lua_pushsstring(L, name);
	lua_pushnumber(L, number);
	lua_rawset(L, -3);
	lua_pushnumber(L, number);
	lua_pushsstring(L, name);
	lua_rawset(L, -3);
}



static inline const char* LuaErrorString(int error)
{
	const char* ret = "";

	switch (error) {
		case LUA_ERRMEM   : { ret = "LUA_ERRMEM"   ; } break;
		case LUA_ERRRUN   : { ret = "LUA_ERRRUN"   ; } break;
		case LUA_ERRERR   : { ret = "LUA_ERRERR"   ; } break;
		case LUA_ERRSYNTAX: { ret = "LUA_ERRSYNTAX"; } break;
		default           : { ret = "LUA_ERRNOIDEA"; } break;
	}

	return ret;
}



static inline bool FullCtrl(const lua_State* L)
{
	return CLuaHandle::GetHandleFullCtrl(L);
}

static inline int CtrlTeam(const lua_State* L)
{
	return CLuaHandle::GetHandleCtrlTeam(L);
}

static inline int CtrlAllyTeam(const lua_State* L)
{
	const int ctrlTeam = CtrlTeam(L);

	if (ctrlTeam < 0)
		return ctrlTeam;

	return teamHandler.AllyTeam(ctrlTeam);
}


static inline bool CanControlTeam(const lua_State* L, int teamID)
{
	const int ctrlTeam = CtrlTeam(L);

	if (ctrlTeam < 0)
		return (ctrlTeam == CEventClient::AllAccessTeam);

	return (ctrlTeam == teamID);
}

static inline bool CanControlAllyTeam(const lua_State* L, int allyTeamID)
{
	const int ctrlTeam = CtrlTeam(L);

	if (ctrlTeam < 0)
		return (ctrlTeam == CEventClient::AllAccessTeam);

	return (teamHandler.AllyTeam(ctrlTeam) == allyTeamID);
}

static inline bool CanControlFeatureAllyTeam(const lua_State* L, int allyTeamID)
{
	const int ctrlTeam = CtrlTeam(L);

	if (ctrlTeam < 0)
		return (ctrlTeam == CEventClient::AllAccessTeam);

	if (allyTeamID < 0)
		return (ctrlTeam == teamHandler.GaiaTeamID());

	return (teamHandler.AllyTeam(ctrlTeam) == allyTeamID);
}

static inline bool CanControlProjectileAllyTeam(const lua_State* L, int allyTeamID)
{
	const int ctrlTeam = CtrlTeam(L);

	if (ctrlTeam < 0)
		return (ctrlTeam == CEventClient::AllAccessTeam);

	if (allyTeamID < 0)
		return false;

	return (teamHandler.AllyTeam(ctrlTeam) == allyTeamID);
}


static inline bool CanControlUnit(const lua_State* L, const CUnit* unit)
{
	const int ctrlTeam = CtrlTeam(L);

	if (ctrlTeam < 0)
		return (ctrlTeam == CEventClient::AllAccessTeam);

	return (ctrlTeam == unit->team);
}

static inline bool CanControlFeature(const lua_State* L, const CFeature* feature)
{
	return CanControlFeatureAllyTeam(L, feature->allyteam);
}


static inline const LocalModelPiece* ParseObjectConstLocalModelPiece(lua_State* L, const CSolidObject* obj, int pieceArg)
{
	assert(obj != nullptr);

	const unsigned int pieceIdx = luaL_checkint(L, pieceArg) - 1;

	// automatically false if LM was not initialized
	if (!obj->localModel.HasPiece(pieceIdx))
		return nullptr;

	return (obj->localModel.GetPiece(pieceIdx));
}

static inline LocalModelPiece* ParseObjectLocalModelPiece(lua_State* L, CSolidObject* obj, int pieceArg)
{
	return (const_cast<LocalModelPiece*>(ParseObjectConstLocalModelPiece(L, obj, pieceArg)));
}

#endif // LUA_UTILS_H
