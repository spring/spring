/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_UTILS_H
#define LUA_UTILS_H

#include <string>
#include <vector>
using std::string;
using std::vector;

#include "LuaHashString.h"
#include "LuaInclude.h"
#include "LuaDefs.h"
#include "Sim/Units/CommandAI/Command.h"

// is defined as macro on FreeBSD (wtf)
#ifdef isnumber
	#undef isnumber
#endif

class LuaUtils {
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
		struct ScopedDebugTraceBack {
		public:
			ScopedDebugTraceBack(lua_State* L);
			~ScopedDebugTraceBack();
			void SetErrFuncIdx(int idx) { errFuncIdx = idx; }
			int GetErrFuncIdx() const { return errFuncIdx; }

		private:
			lua_State* luaState;

			int errFuncIdx;
		};

		static int exportedDataSize; //< performance stat

		static int Backup(std::vector<DataDump> &backup, lua_State* src, int count);

		static int Restore(const std::vector<DataDump> &backup, lua_State* dst);

		static int ShallowBackup(std::vector<ShallowDataDump> &backup, lua_State* src, int count);

		static int ShallowRestore(const std::vector<ShallowDataDump> &backup, lua_State* dst);

		static int CopyData(lua_State* dst, lua_State* src, int count);

		static void PushCurrentFuncEnv(lua_State* L, const char* caller);

		static int PushDebugTraceback(lua_State* L); //< returns stack index of traceback function

		// lower case all keys in the table, with recursion
		static bool LowerKeys(lua_State* L, int tableIndex);

		static void PushCommandParamsTable(lua_State* L, const Command& cmd, bool subtable);
		static void PushCommandOptionsTable(lua_State* L, const Command& cmd, bool subtable);
		// from LuaUI.cpp / LuaSyncedCtrl.cpp (used to be duplicated)
		static void ParseCommandOptions(
			lua_State* L,
			Command& cmd,
			const char* caller,
			const int idx
		);
		static Command ParseCommand(lua_State* L, const char* caller,
				int idIndex);
		static Command ParseCommandTable(lua_State* L, const char* caller,
				int table);
		static void ParseCommandArray(lua_State* L, const char* caller,
		                              int table, vector<Command>& commands);
		static int ParseFacing(lua_State* L, const char* caller, int index);

		static void* GetUserData(lua_State* L, int index, const string& type);

		static void PrintStack(lua_State* L);

		// from LuaFeatureDefs.cpp / LuaUnitDefs.cpp / LuaWeaponDefs.cpp
		// (helper for the Next() iteration routine)
		static int Next(const ParamMap& paramMap, lua_State* L);

		// from LuaParser.cpp / LuaUnsyncedCtrl.cpp
		// (implementation copied from lua/src/lib/lbaselib.c)
		static int Echo(lua_State* L);
		static int Log(lua_State* L);
		static bool PushLogEntries(lua_State* L);

		static int ZlibCompress(lua_State* L);
		static int ZlibDecompress(lua_State* L);

		static bool PushCustomBaseFunctions(lua_State* L);
		static int tobool(lua_State* L);
		static int isnil(lua_State* L);
		static int isbool(lua_State* L);
		static int isnumber(lua_State* L);
		static int isstring(lua_State* L);
		static int istable(lua_State* L);
		static int isthread(lua_State* L);
		static int isfunction(lua_State* L);
		static int isuserdata(lua_State* L);

		// not implemented (except for the first two)...
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

		static void PushCommandDesc(lua_State* L, const CommandDescription& cd);
};


inline void LuaPushNamedBool(lua_State* L,
                             const string& key, bool value)
{
	lua_pushsstring(L, key);
	lua_pushboolean(L, value);
	lua_rawset(L, -3);
}


inline void LuaPushNamedNumber(lua_State* L,
                               const string& key, lua_Number value)
{
	lua_pushsstring(L, key);
	lua_pushnumber(L, value);
	lua_rawset(L, -3);
}


inline void LuaPushNamedString(lua_State* L,
                               const string& key, const string& value)
{
	lua_pushsstring(L, key);
	lua_pushsstring(L, value);
	lua_rawset(L, -3);
}


inline void LuaPushNamedCFunc(lua_State* L,
                              const string& key, int (*func)(lua_State*))
{
	lua_pushsstring(L, key);
	lua_pushcfunction(L, func);
	lua_rawset(L, -3);
}


inline void LuaInsertDualMapPair(lua_State* L, const string& name, int number)
{
	lua_pushsstring(L, name);
	lua_pushnumber(L, number);
	lua_rawset(L, -3);
	lua_pushnumber(L, number);
	lua_pushsstring(L, name);
	lua_rawset(L, -3);
}

#endif // LUA_UTILS_H
