/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_OBJECT_RENDERING_H
#define LUA_OBJECT_RENDERING_H

// LUAOBJ_*
#include "LuaObjectMaterial.h"

struct lua_State;

template<LuaObjType T> class LuaObjectRendering;

class LuaObjectRenderingImpl {
public:
	static bool PushEntries(lua_State* L);
	static void PushObjectType(LuaObjType type) { objectTypeStack.push_back(type); }
	static void PopObjectType() { objectTypeStack.pop_back(); }

private:
	friend class LuaObjectRendering<LUAOBJ_UNIT>;
	friend class LuaObjectRendering<LUAOBJ_FEATURE>;

	static LuaObjType GetObjectType() {
		if (!objectTypeStack.empty())
			return (objectTypeStack.back());

		return LUAOBJ_LAST;
	}

	static int SetLODCount(lua_State* L);
	static int SetLODLength(lua_State* L);
	static int SetLODDistance(lua_State* L);

	// LUAOBJ_UNIT only
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

private:
	static std::vector<LuaObjType> objectTypeStack;
};



template<LuaObjType T> class LuaObjectRendering {
public:
	static bool PushEntries(lua_State* L) {
		return (LuaObjectRenderingImpl::PushEntries(L));
	}

private:
	static int SetLODCount(lua_State* L) {
		LuaObjectRenderingImpl::PushObjectType(T);
		const int ret = LuaObjectRenderingImpl::SetLODCount(L);
		LuaObjectRenderingImpl::PopObjectType();
		return ret;
	}
	static int SetLODLength(lua_State* L) {
		LuaObjectRenderingImpl::PushObjectType(T);
		const int ret = LuaObjectRenderingImpl::SetLODLength(L);
		LuaObjectRenderingImpl::PopObjectType();
		return ret;
	}
	static int SetLODDistance(lua_State* L) {
		LuaObjectRenderingImpl::PushObjectType(T);
		const int ret = LuaObjectRenderingImpl::SetLODDistance(L);
		LuaObjectRenderingImpl::PopObjectType();
		return ret;
	}


	static int SetPieceList(lua_State* L) {
		if (T != LUAOBJ_UNIT)
			return 0;

		return (LuaObjectRenderingImpl::SetPieceList(L));
	}


	static int GetMaterial(lua_State* L) {
		LuaObjectRenderingImpl::PushObjectType(T);
		const int ret = LuaObjectRenderingImpl::GetMaterial(L);
		LuaObjectRenderingImpl::PopObjectType();
		return ret;
	}
	static int SetMaterial(lua_State* L) {
		LuaObjectRenderingImpl::PushObjectType(T);
		const int ret = LuaObjectRenderingImpl::SetMaterial(L);
		LuaObjectRenderingImpl::PopObjectType();
		return ret;
	}

	static int SetMaterialLastLOD(lua_State* L) {
		LuaObjectRenderingImpl::PushObjectType(T);
		const int ret = LuaObjectRenderingImpl::SetMaterialLastLOD(L);
		LuaObjectRenderingImpl::PopObjectType();
		return ret;
	}
	static int SetMaterialDisplayLists(lua_State* L) {
		LuaObjectRenderingImpl::PushObjectType(T);
		const int ret = LuaObjectRenderingImpl::SetMaterialDisplayLists(L);
		LuaObjectRenderingImpl::PopObjectType();
		return ret;
	}

	static int SetObjectUniform(lua_State* L) {
		LuaObjectRenderingImpl::PushObjectType(T);
		const int ret = LuaObjectRenderingImpl::SetObjectUniform(L);
		LuaObjectRenderingImpl::PopObjectType();
		return ret;
	}


	static int SetUnitLuaDraw(lua_State* L) {
		if (T != LUAOBJ_UNIT)
			return 0;

		return (LuaObjectRenderingImpl::SetUnitLuaDraw(L));
	}
	static int SetFeatureLuaDraw(lua_State* L) {
		if (T != LUAOBJ_FEATURE)
			return 0;

		return (LuaObjectRenderingImpl::SetFeatureLuaDraw(L));
	}


	static int Debug(lua_State* L) {
		LuaObjectRenderingImpl::PushObjectType(T);
		const int ret = LuaObjectRenderingImpl::Debug(L);
		LuaObjectRenderingImpl::PopObjectType();
		return ret;
	}
};

#endif /* LUA_OBJECT_RENDERING_H */

