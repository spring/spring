#ifndef LUA_UNIT_RENDERING_H
#define LUA_UNIT_RENDERING_H
// LuaUnitRendering.h: interface for the LuaUnitRendering class.
//
//////////////////////////////////////////////////////////////////////

struct lua_State;


class LuaUnitRendering {
	public:
		static bool PushEntries(lua_State* L);

	private:
		static int SetLODCount(lua_State* L);
		static int SetLODLength(lua_State* L);
		static int SetLODDistance(lua_State* L);

		static int SetPieceList(lua_State* L);

		static int GetMaterial(lua_State* L);
		static int SetMaterial(lua_State* L);
		static int SetMaterialLastLOD(lua_State* L);
		static int SetMaterialDisplayLists(lua_State* L);

		static int CreateGlobalUniforms(lua_State* L);
		static int SetGlobalUniform(lua_State* L);

		static int CreateUnitUniforms(lua_State* L);
		static int SetUnitUniform(lua_State* L);

		static int SetUnitLuaDraw(lua_State* L);

		static int Debug(lua_State* L);
};


#endif /* LUA_UNIT_RENDERING_H */
