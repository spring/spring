#ifndef LUA_UNITDEFS_H
#define LUA_UNITDEFS_H
// LuaUnitDefs.h: interface for the LuaUnitDefs class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include <string>


struct lua_State;

class LuaUnitDefs {
	public:
		static bool PushEntries(lua_State* L);

		static bool IsDefaultParam(const std::string& word);
};


#endif /* LUA_UNITDEFS_H */
