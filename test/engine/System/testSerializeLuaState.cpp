/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/creg/Serializer.h"
#include "System/creg/SerializeLuaState.h"

#define BOOST_TEST_MODULE SerializeLuaState
#include <boost/test/unit_test.hpp>


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


struct LuaRoot {
	CR_DECLARE_STRUCT(LuaRoot);
	lua_State* L;
	void Serialize(creg::ISerializer* s);
};

CR_BIND(LuaRoot, );
CR_REG_METADATA(LuaRoot, (
	CR_IGNORED(L),
	CR_SERIALIZER(Serialize)
))


void LuaRoot::Serialize(creg::ISerializer* s) {
	creg::SerializeLuaState(s, &L);
}

BOOST_AUTO_TEST_CASE( SerializeLuaState )
{
	// int context = 1;
	// creg::SetLuaContext(&context, l_alloc, handlepanic);


	// lua_State* L = lua_newstate(l_alloc, &context);
	// lua_atpanic(L, handlepanic);

	// LuaRoot root = {L};
	// std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);


	// creg::COutputStreamSerializer oser;

	// oser.SavePackage(&ss, &root, root.GetClass());

	// lua_close(L);



	// creg::CInputStreamSerializer iser;

	// void* loaded;
	// creg::Class* loadedCls;
	// iser.LoadPackage(&ss, loaded, loadedCls);

	// LuaRoot* loadedRoot = (LuaRoot*) loaded;

	// lua_close(loadedRoot->L);
	// delete loadedRoot;

	BOOST_CHECK_MESSAGE(true, "Failed lua serialization!");
}