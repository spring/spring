#ifndef LUA_COB_H
#define LUA_COB_H
// LuaCob.h: interface for the CLuaCob class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include <string>
#include <vector>
using std::string;
using std::vector;

#include "LuaHandleSynced.h"


class CUnit;
struct lua_State;
struct LuaHashString;


#define MAX_LUA_COB_ARGS 10


class CLuaCob : public CLuaHandleSynced
{
	public:
		static void LoadHandler();
		static void FreeHandler();

		void CallFunction(const LuaHashString& funcName, const CUnit* unit,
		                  int& argsCount, int args[MAX_LUA_COB_ARGS]);

	protected:
		bool AddSyncedCode();
		bool AddUnsyncedCode() { return true; }

	private:
		CLuaCob();
		~CLuaCob();

	private:
		static const int* currentArgs;

	private:
		static int UnpackCobArg(lua_State* L);
		static void CobCallback(int retCode, void* p1, void* p2);
};


extern CLuaCob* luaCob;


#endif /* LUA_COB_H */
