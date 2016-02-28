/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_RBOS_H
#define LUA_RBOS_H

#include <set>
using std::set;

#include "Rendering/GL/myGL.h"


struct lua_State;


class LuaRBOs {
	public:
		LuaRBOs();
		~LuaRBOs();

		static bool PushEntries(lua_State* L);

		struct RBO;
		static const RBO* GetLuaRBO(lua_State* L, int index);

	public:
		struct RBO {
			RBO() : id(0), target(0), format(0), xsize(0), ysize(0) {}
			void Init();
			void Free(lua_State* L);

			GLuint id;
			GLenum target;
			GLenum format;
			GLsizei xsize;
			GLsizei ysize;
		};

	private:
		set<RBO*> rbos;

	private: // helpers
		static bool CreateMetatable(lua_State* L);

	private: // metatable methods
		static int meta_gc(lua_State* L);
		static int meta_index(lua_State* L);
		static int meta_newindex(lua_State* L);

	private:
		static int CreateRBO(lua_State* L);
		static int DeleteRBO(lua_State* L);
};


#endif /* LUA_RBOS_H */
