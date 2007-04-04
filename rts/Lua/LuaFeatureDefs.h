#ifndef LUA_FEATUREDEFS_H
#define LUA_FEATUREDEFS_H
// LuaFeatureDefs.h: interface for the LuaFeatureDefs class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include <string>


struct lua_State;

class LuaFeatureDefs {
	public:
		static bool PushEntries(lua_State* L);

		static bool IsDefaultParam(const std::string& word);
};


#endif /* LUA_FEATUREDEFS_H */
