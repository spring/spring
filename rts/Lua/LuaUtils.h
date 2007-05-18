#ifndef LUA_UTILS_H
#define LUA_UTILS_H
// LuaUtils.h: lua utility routines
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include <string>
#include <vector>
using std::string;
using std::vector;

#include "LuaHashString.h"

#include "LuaInclude.h"

class LuaUtils {
	public:
		static int CopyData(lua_State* dst, lua_State* src, int count);

		// not implemented...		
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
};


inline void LuaPushNamedBool(lua_State* L,
                             const string& key, bool value)
{
	lua_pushstring(L, key.c_str());
	lua_pushboolean(L, value);
	lua_rawset(L, -3);
}


inline void LuaPushNamedNumber(lua_State* L,
                               const string& key, lua_Number value)
{
	lua_pushstring(L, key.c_str());
	lua_pushnumber(L, value);
	lua_rawset(L, -3);
}


inline void LuaPushNamedString(lua_State* L,
                               const string& key, const string& value)
{
	lua_pushstring(L, key.c_str());
	lua_pushstring(L, value.c_str());
	lua_rawset(L, -3);
}


inline void LuaPushNamedCFunc(lua_State* L,
                              const string& key, int (*func)(lua_State*))
{
	lua_pushstring(L, key.c_str());
	lua_pushcfunction(L, func);
	lua_rawset(L, -3);
}


#endif // LUA_UTILS_H

