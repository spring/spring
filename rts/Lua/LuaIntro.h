/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_LOADSCREEN_H
#define LUA_LOADSCREEN_H

#include "LuaHandle.h"


struct lua_State;


class CLuaIntro : public CLuaHandle
{
	public:
		static void LoadHandler();
		static void FreeHandler();

	public: // call-ins
		bool HasCallIn(lua_State *L, const std::string& name);
		bool UnsyncedUpdateCallIn(lua_State *L, const std::string& name);

		void Shutdown();
		void DrawLoadScreen();
		void LoadProgress(const std::string& msg, const bool replace_lastline);

	protected:
		CLuaIntro();
		virtual ~CLuaIntro();

		std::string LoadFile(const std::string& filename) const;

	private:
		static bool LoadUnsyncedCtrlFunctions(lua_State *L);
		static bool LoadUnsyncedReadFunctions(lua_State *L);
		static bool LoadSyncedReadFunctions(lua_State *L);
		static bool RemoveSomeOpenGLFunctions(lua_State *L);
};


extern CLuaIntro* LuaIntro;


#endif /* LUA_LOADSCREEN_H */
