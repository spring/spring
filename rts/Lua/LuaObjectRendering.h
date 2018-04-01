/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_OBJECT_RENDERING_H
#define LUA_OBJECT_RENDERING_H

// for LuaObjType
#include "LuaObjectMaterial.h"

struct lua_State;

template<LuaObjType T> class LuaObjectRendering;

class LuaObjectRenderingImpl {
private:
	friend class LuaObjectRendering<LUAOBJ_UNIT>;
	friend class LuaObjectRendering<LUAOBJ_FEATURE>;

	static void CreateMatRefMetatable(lua_State* L);
	static void PushFunction(lua_State* L, int (*fnPntr)(lua_State*), const char* fnName);

	static void PushObjectType(LuaObjType type) { objectTypeStack.push_back(type); }
	static void PopObjectType() { objectTypeStack.pop_back(); }

	static LuaObjType GetObjectType() {
		if (!objectTypeStack.empty())
			return (objectTypeStack.back());

		return LUAOBJ_LAST;
	}

	static int GetLODCount(lua_State* L);
	static int SetLODCount(lua_State* L);
	static int SetLODLength(lua_State* L);
	static int SetLODDistance(lua_State* L);

	static int SetPieceList(lua_State* L) { return 0; }

	static int GetMaterial(lua_State* L);
	static int SetMaterial(lua_State* L);
	static int SetMaterialLastLOD(lua_State* L);
	static int SetMaterialDisplayLists(lua_State* L) { return 0; }

	static int SetUnitLuaDraw(lua_State* L);
	static int SetFeatureLuaDraw(lua_State* L);
	static int SetProjectileLuaDraw(lua_State* L);

	static int Debug(lua_State* L);

private:
	static std::vector<LuaObjType> objectTypeStack;
};



template<LuaObjType T> class LuaObjectRendering {
public:
	static bool PushEntries(lua_State* L) {
		LuaObjectRenderingImpl::CreateMatRefMetatable(L);

		// register our wrapper functions so the implementations know
		// the proper LuaObjType value for their corresponding tables
		#define PUSH_FUNCTION(x) LuaObjectRenderingImpl::PushFunction(L, x, #x)

		PUSH_FUNCTION(GetLODCount);
		PUSH_FUNCTION(SetLODCount);
		PUSH_FUNCTION(SetLODLength);
		PUSH_FUNCTION(SetLODDistance);

		PUSH_FUNCTION(GetMaterial);
		PUSH_FUNCTION(SetMaterial);

		PUSH_FUNCTION(SetMaterialLastLOD);
		PUSH_FUNCTION(SetMaterialDisplayLists);

		PUSH_FUNCTION(SetPieceList);

		PUSH_FUNCTION(SetUnitLuaDraw);
		PUSH_FUNCTION(SetFeatureLuaDraw);
		PUSH_FUNCTION(SetProjectileLuaDraw);

		PUSH_FUNCTION(Debug);

		#undef PUSH_FUNCTION
		return true;
	}

private:
	static int GetLODCount(lua_State* L) {
		LuaObjectRenderingImpl::PushObjectType(T);
		const int ret = LuaObjectRenderingImpl::GetLODCount(L);
		LuaObjectRenderingImpl::PopObjectType();
		return ret;
	}
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
		LuaObjectRenderingImpl::PushObjectType(T);
		const int ret = LuaObjectRenderingImpl::SetPieceList(L);
		LuaObjectRenderingImpl::PopObjectType();
		return ret;
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
	// NB: value of T is not relevant here, slightly foreign callout
	static int SetProjectileLuaDraw(lua_State* L) {
		return (LuaObjectRenderingImpl::SetProjectileLuaDraw(L));
	}


	static int Debug(lua_State* L) {
		LuaObjectRenderingImpl::PushObjectType(T);
		const int ret = LuaObjectRenderingImpl::Debug(L);
		LuaObjectRenderingImpl::PopObjectType();
		return ret;
	}
};

#endif /* LUA_OBJECT_RENDERING_H */

