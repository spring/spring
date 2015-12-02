/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaRBOs.h"

#include "LuaInclude.h"

#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaUtils.h"


/******************************************************************************/
/******************************************************************************/

LuaRBOs::LuaRBOs()
{
}


LuaRBOs::~LuaRBOs()
{
	set<RBO*>::const_iterator it;
	for (it = rbos.begin(); it != rbos.end(); ++it) {
		const RBO* rbo = *it;
		glDeleteRenderbuffersEXT(1, &rbo->id);
	}
}


/******************************************************************************/
/******************************************************************************/

bool LuaRBOs::PushEntries(lua_State* L)
{
	CreateMetatable(L);

#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(CreateRBO);
	REGISTER_LUA_CFUNC(DeleteRBO);

	return true;
}


bool LuaRBOs::CreateMetatable(lua_State* L)
{
	luaL_newmetatable(L, "RBO");
	HSTR_PUSH_CFUNC(L, "__gc",        meta_gc);
	HSTR_PUSH_CFUNC(L, "__index",     meta_index);
	HSTR_PUSH_CFUNC(L, "__newindex",  meta_newindex);
	lua_pop(L, 1);
	return true;
}


/******************************************************************************/
/******************************************************************************/

const LuaRBOs::RBO* LuaRBOs::GetLuaRBO(lua_State* L, int index)
{
	return static_cast<RBO*>(LuaUtils::GetUserData(L, index, "RBO"));
}


/******************************************************************************/
/******************************************************************************/

void LuaRBOs::RBO::Init()
{
	id     = 0;
	target = GL_RENDERBUFFER_EXT;
	format = GL_RGBA;
	xsize  = 0;
	ysize  = 0;
}


void LuaRBOs::RBO::Free(lua_State* L)
{
	if (id == 0) {
		return;
	}

	glDeleteRenderbuffersEXT(1, &id);
	id = 0;

	CLuaHandle::GetActiveRBOs(L).rbos.erase(this);
}


/******************************************************************************/
/******************************************************************************/

int LuaRBOs::meta_gc(lua_State* L)
{
	RBO* rbo = static_cast<RBO*>(luaL_checkudata(L, 1, "RBO"));
	rbo->Free(L);
	return 0;
}


int LuaRBOs::meta_index(lua_State* L)
{
	const RBO* rbo = static_cast<RBO*>(luaL_checkudata(L, 1, "RBO"));
	const string key = luaL_checkstring(L, 2);
	if (key == "valid") {
		lua_pushboolean(L, glIsRenderbufferEXT(rbo->id));
	}
	else if (key == "target") { lua_pushnumber(L, rbo->target); }
	else if (key == "format") { lua_pushnumber(L, rbo->format); }
	else if (key == "xsize")  { lua_pushnumber(L, rbo->xsize);  }
	else if (key == "ysize")  { lua_pushnumber(L, rbo->ysize);  }
	else {
		return 0;
	}
	return 1;
}


int LuaRBOs::meta_newindex(lua_State* L)
{
	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaRBOs::CreateRBO(lua_State* L)
{
	RBO rbo;
	rbo.Init();

	rbo.xsize = (GLsizei)luaL_checknumber(L, 1);
	rbo.ysize = (GLsizei)luaL_checknumber(L, 2);
	rbo.target = GL_RENDERBUFFER_EXT;
	rbo.format = GL_RGBA;

	const int table = 3;
	if (lua_istable(L, table)) {
		lua_getfield(L, table, "target");
		if (lua_isnumber(L, -1)) {
			rbo.target = (GLenum)lua_tonumber(L, -1);
		}
		lua_pop(L, 1);
		lua_getfield(L, table, "format");
		if (lua_isnumber(L, -1)) {
			rbo.format = (GLenum)lua_tonumber(L, -1);
		}
		lua_pop(L, 1);
	}

	glGenRenderbuffersEXT(1, &rbo.id);
	glBindRenderbufferEXT(rbo.target, rbo.id);

	// allocate the memory
	glRenderbufferStorageEXT(rbo.target, rbo.format, rbo.xsize, rbo.ysize);

	glBindRenderbufferEXT(rbo.target, 0);

	RBO* rboPtr = static_cast<RBO*>(lua_newuserdata(L, sizeof(RBO)));
	*rboPtr = rbo;

	luaL_getmetatable(L, "RBO");
	lua_setmetatable(L, -2);

	if (rboPtr->id != 0) {
		CLuaHandle::GetActiveRBOs(L).rbos.insert(rboPtr);
	}

	return 1;
}


int LuaRBOs::DeleteRBO(lua_State* L)
{
	if (lua_isnil(L, 1)) {
		return 0;
	}
	RBO* rbo = static_cast<RBO*>(luaL_checkudata(L, 1, "RBO"));
	rbo->Free(L);
	return 0;
}


/******************************************************************************/
/******************************************************************************/
