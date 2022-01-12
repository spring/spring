/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/creg/Serializer.h"
#include "System/creg/SerializeLuaState.h"

#define CATCH_CONFIG_MAIN
#include "lib/catch.hpp"


static int handlepanic(lua_State* L)
{
	throw "lua paniced";
}


// from lauxlib.cpp
static void* l_alloc (void* ud, void* ptr, size_t osize, size_t nsize) {
	(void)ud;
	(void)osize;
	if (nsize == 0) {
		free(ptr);
		return NULL;
	} else {
		return realloc(ptr, nsize);
	}
}

struct FakeLuaHandle {
	lua_State* L = nullptr;
	lua_State* L_GC = nullptr;
};

FakeLuaHandle flh;


struct LuaRoot {
	CR_DECLARE_STRUCT(LuaRoot);
	void Serialize(creg::ISerializer* s);
};

CR_BIND(LuaRoot, );
CR_REG_METADATA(LuaRoot, (
	CR_SERIALIZER(Serialize)
))


void LuaRoot::Serialize(creg::ISerializer* s) {
	creg::SerializeLuaState(s, &flh.L);
	creg::SerializeLuaThread(s, &flh.L_GC);
}

TEST_CASE("SerializeLuaState")
{
	int context = 1;


	flh.L = lua_newstate(l_alloc, &context);
	lua_atpanic(flh.L, handlepanic);
	SPRING_LUA_OPEN_LIB(flh.L, luaopen_base);
	SPRING_LUA_OPEN_LIB(flh.L, luaopen_math);
	SPRING_LUA_OPEN_LIB(flh.L, luaopen_table);
	SPRING_LUA_OPEN_LIB(flh.L, luaopen_string);

	lua_settop(flh.L, 0);
	creg::AutoRegisterCFunctions("Test::", flh.L);
	flh.L_GC = lua_newthread(flh.L);
	int idx = luaL_ref(flh.L, LUA_REGISTRYINDEX);
	const char* code = "local co = coroutine.create(function ()\n local function f()\n coroutine.yield()\n end\n f()\n end);\ncoroutine.resume(co);\n";

	int err = luaL_loadbuffer(flh.L, code, strlen(code), "yield");
	if (err)
	{
		printf("%s\n", lua_tostring(flh.L, -1));
		lua_pop(flh.L, 1);
	}
	lua_pcall(flh.L, 0, 0, 0);

	std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
	{
		LuaRoot root;
		creg::COutputStreamSerializer oser;
		oser.SavePackage(&ss, &root, root.GetClass());
	}

	{
		creg::CInputStreamSerializer iser;
		void* loaded;
		creg::Class* loadedCls;
		creg::CopyLuaContext(flh.L);
		LUA_CLOSE(&flh.L);
		iser.LoadPackage(&ss, loaded, loadedCls);
		LuaRoot* loadedRoot = (LuaRoot*) loaded;
		delete loadedRoot;
	}

	lua_rawgeti(flh.L, LUA_REGISTRYINDEX, idx);
	lua_State* L_GC = lua_tothread(flh.L, -1);
	CHECK(L_GC == flh.L_GC);

	lua_close(flh.L);
}