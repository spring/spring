#ifndef LUA_FBOS_H
#define LUA_FBOS_H
// LuaFBOs.h: interface for the LuaFBOs class.
//
//////////////////////////////////////////////////////////////////////


#include <set>
#include <string>
using std::set;
using std::string;

#include "Rendering/GL/myGL.h"

struct lua_State;


class LuaFBOs {
	public:
		LuaFBOs();
		~LuaFBOs();

		struct FBO;
		const FBO* GetLuaFBO(lua_State* L, int index);
		const void* GetMetatable() const { return metatable; }

	public:
		static bool PushEntries(lua_State* L);

		static GLenum ParseAttachment(const string& name);

	public:
		struct FBO {
			void Init(lua_State* L);
			void Free(lua_State* L);

			GLuint id;
			GLenum target;
			int luaRef;
			GLsizei xsize;
			GLsizei ysize;
		};

	private:
		set<FBO*> fbos;
		const void* metatable;

	private: // helpers
		static bool CreateMetatable(lua_State* L);
		static bool AttachObject(lua_State* L, int index,
		                         FBO* fbo, GLenum attachID,
		                         GLenum attachTarget = 0,
		                         GLenum attachLevel  = 0);
		static bool ApplyAttachment(lua_State* L, int index,
		                            FBO* fbo, GLenum attachID);
		static bool ApplyDrawBuffers(lua_State* L, int index);

	private: // metatable methods
		static int meta_gc(lua_State* L);
		static int meta_index(lua_State* L);
		static int meta_newindex(lua_State* L);

	private: // call-outs
		static int CreateFBO(lua_State* L);
		static int DeleteFBO(lua_State* L);
		static int IsValidFBO(lua_State* L);
		static int UseFBO(lua_State* L);
		static int UnsafeSetFBO(lua_State* L); // unsafe
		static int BlitFBO(lua_State* L);
};


#endif /* LUA_FBOS_H */
