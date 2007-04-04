#ifndef LUA_GAIA_H
#define LUA_GAIA_H
// LuaGaia.h: interface for the CLuaGaia class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include <string>
using std::string;

#include "LuaHandleSynced.h"


class CLuaGaia : public CLuaHandleSynced
{
	public:
		static void LoadHandler();
		static void FreeHandler();

	protected:
		bool AddSyncedCode() { return true; }
		bool AddUnsyncedCode() { return true; }

	private:
		CLuaGaia();
		~CLuaGaia();

	private:
		static void CobCallback(int retCode, void* p1, void* p2);
};


extern CLuaGaia* luaGaia;


#endif /* LUA_GAIA_H */
