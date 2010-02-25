/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_GAIA_H
#define LUA_GAIA_H

#include <string>
using std::string;

#include "LuaHandleSynced.h"


class CLuaGaia : public CLuaHandleSynced
{
	public:
		static void LoadHandler();
		static void FreeHandler();

		static bool SetConfigString(const string& cfg);
		static const string& GetConfigString() { return configString; }

	protected:
		bool AddSyncedCode();
		bool AddUnsyncedCode();

	private:
		CLuaGaia();
		~CLuaGaia();

	private: // call-outs
		static int GetConfigString(lua_State* L);

	private:
		static string configString;
};


extern CLuaGaia* luaGaia;


#endif /* LUA_GAIA_H */
