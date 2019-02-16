/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaRBOs.h"

#include "LuaInclude.h"

#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaUtils.h"

#include "System/myMath.h"

#include "Rendering/GlobalRendering.h"


/******************************************************************************/
/******************************************************************************/

LuaRBOs::~LuaRBOs()
{
	for (const RBO* rbo: rbos) {
		glDeleteRenderbuffersEXT(1, &rbo->id);
	}
}


/******************************************************************************/
/******************************************************************************/

bool LuaRBOs::PushEntries(lua_State* L)
{
	CreateMetatable(L);

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
	index  = -1u;
	id     = 0;
	target = GL_RENDERBUFFER_EXT;
	format = GL_RGBA;
	xsize  = 0;
	ysize  = 0;

	multisample = false;
}


void LuaRBOs::RBO::Free(lua_State* L)
{
	if (id == 0)
		return;

	glDeleteRenderbuffersEXT(1, &id);
	id = 0;

	{
		// get rid of the userdatum
		LuaRBOs& activeRBOs = CLuaHandle::GetActiveRBOs(L);
		auto& rbos = activeRBOs.rbos;

		assert(index < rbos.size());
		assert(rbos[index] == this);

		rbos[index] = rbos.back();
		rbos[index]->index = index;
		rbos.pop_back();
	}
}


/******************************************************************************/
/******************************************************************************/

int LuaRBOs::meta_gc(lua_State* L)
{
	auto rbo = static_cast<RBO*>(luaL_checkudata(L, 1, "RBO"));
	rbo->Free(L);
	return 0;
}


int LuaRBOs::meta_index(lua_State* L)
{
	const RBO* rbo = static_cast<RBO*>(luaL_checkudata(L, 1, "RBO"));
	const std::string& key = luaL_checkstring(L, 2);

	if (key ==  "valid") { lua_pushboolean(L, glIsRenderbufferEXT(rbo->id)); return 1; }
	if (key == "target") { lua_pushnumber(L, rbo->target); return 1; }
	if (key == "format") { lua_pushnumber(L, rbo->format); return 1; }
	if (key ==  "xsize") { lua_pushnumber(L, rbo->xsize ); return 1; }
	if (key ==  "ysize") { lua_pushnumber(L, rbo->ysize ); return 1; }
	if (key ==  "multisample") { lua_pushboolean(L, rbo->multisample); return 1; }

	return 0;
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
		lua_getfield(L, table, "multisample");
		if (lua_isboolean(L, -1)) {
			rbo.multisample = lua_toboolean(L, -1);
		}
		lua_pop(L, 1);
	}

	glGenRenderbuffersEXT(1, &rbo.id);
	glBindRenderbufferEXT(rbo.target, rbo.id);

	// allocate the memory
	// In theory glRenderbufferStorageEXT is an equivalent of glRenderbufferStorageMultisample(...,samples = 0,...), so these two calls be replaced with one later on.
	if (rbo.multisample)
		glRenderbufferStorageMultisample(rbo.target, globalRendering->msaaLevel, rbo.format, rbo.xsize, rbo.ysize);
	else
		glRenderbufferStorageEXT(rbo.target, rbo.format, rbo.xsize, rbo.ysize);

	glBindRenderbufferEXT(rbo.target, 0);

	auto rboPtr = static_cast<RBO*>(lua_newuserdata(L, sizeof(RBO)));
	*rboPtr = rbo;

	luaL_getmetatable(L, "RBO");
	lua_setmetatable(L, -2);

	if (rboPtr->id != 0) {
		LuaRBOs& activeRBOs = CLuaHandle::GetActiveRBOs(L);
		auto& rbos = activeRBOs.rbos;

		rbos.push_back(rboPtr);
		rboPtr->index = rbos.size() - 1;
	}

	return 1;
}


int LuaRBOs::DeleteRBO(lua_State* L)
{
	if (lua_isnil(L, 1)) {
		return 0;
	}
	auto rbo = static_cast<RBO*>(luaL_checkudata(L, 1, "RBO"));
	rbo->Free(L);
	return 0;
}


/******************************************************************************/
/******************************************************************************/
