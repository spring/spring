#ifndef LUA_STATE_H
#define LUA_STATE_H
// LuaState.h: interface for the CLuaState class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <string>
#include <set>
using namespace std;


extern "C" {
	#include "lua.h"
}


class CLuaState {

	public:
		~CLuaState();
		
		static CLuaState& GetSingleton();
		
		lua_State* GetL() { return L; }

		const set<string>& GetUnitDefParams() const { return unitDefParams; }
		
	private:
		CLuaState();

		void Init();		
		bool LoadCode(const string& code, const string& debug);
		bool LoadInfo();
		bool LoadGameInfo();
		bool LoadUnitDefInfo();
		bool LoadWeaponDefInfo();
		
	private:
		bool inited;
		lua_State*  L;
		set<string> unitDefParams;
};


#define LUASTATE (CLuaState::GetSingleton())


#endif /* LUA_STATE_H */
