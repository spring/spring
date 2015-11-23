/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_OBJECT_RENDERING_H
#define LUA_OBJECT_RENDERING_H

struct lua_State;
class CUnit;
class CFeature;

class LuaObjectRendering {
public:
	static bool PushEntries(lua_State* L);

private:
	static int SetLODCount(lua_State* L);
	static int SetLODLength(lua_State* L);
	static int SetLODDistance(lua_State* L);

	// CUnit only
	static int SetPieceList(lua_State* L);

	static int GetMaterial(lua_State* L);
	static int SetMaterial(lua_State* L);
	static int SetMaterialLastLOD(lua_State* L);
	static int SetMaterialDisplayLists(lua_State* L);

	// not implemented yet
	static int SetObjectUniform(lua_State* L);

	static int SetUnitLuaDraw(lua_State* L);
	static int SetFeatureLuaDraw(lua_State* L);

	static int Debug(lua_State* L);
};

#endif /* LUA_OBJECT_RENDERING_H */
